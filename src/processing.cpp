#include <pch.h>

#include "hooks.h"
#include "unreal.h"

using namespace ohl::unreal;

namespace ohl::processing {

static std::vector<std::pair<std::wstring, std::wstring>> INJECTED_HOTFIXES = {
    {L"SparkPatchEntry999999",
     L"(1,1,0,),/Game/PlayerCharacters/_Shared/_Design/Sliding/"
     L"ControlledMove_Global_Sliding.Default__ControlledMove_Global_Sliding_C,Duration."
     L"BaseValueConstant,0,,5000"},
};

static std::wstring NEWS_HEADER = L"OHL: Loaded 1234 hotfixes";
static std::wstring NEWS_BODY = L"Running mods:\nA\nB\nC";
static const std::wstring NEWS_IMAGE = L"https://i.ytimg.com/vi/V4MF2s6MLxY/maxresdefault.jpg";

struct vf_tables {
    bool found;
    void* json_value_string;
    void* json_value_array;
    void* json_value_object;
    void* shared_ptr_json_object;
    void* shared_ptr_json_value;
};

static vf_tables vf_table = {};

// Discovery happens first so gather vftables from it
static void gather_vf_tables(FJsonObject* discovery_json) {
    if (vf_table.found) {
        return;
    }

    auto services = discovery_json->get<FJsonValueArray>(L"services");
    vf_table.json_value_array = services->vf_table;
    vf_table.shared_ptr_json_value = services->entries.data[0].ref_controller->vf_table;

    auto first_service = services->get<FJsonValueObject>(0);
    vf_table.json_value_object = first_service->vf_table;
    vf_table.shared_ptr_json_object = first_service->value.ref_controller->vf_table;

    auto group = first_service->to_obj()->get<FJsonValueString>(L"configuration_group");
    vf_table.json_value_string = group->vf_table;

    vf_table.found = true;
}

template <typename T>
static void add_ref_counter(TSharedPtr<T>* ptr, void* vf_table) {
    ptr->ref_controller =
        ohl::hooks::malloc<FReferenceControllerBase>(sizeof(FReferenceControllerBase));
    ptr->ref_controller->vf_table = vf_table;
    ptr->ref_controller->ref_count = 1;
    ptr->ref_controller->weak_ref_count = 1;
    ptr->ref_controller->obj = ptr->obj;
}

static void alloc_string(FString* str, std::wstring value) {
    str->count = value.size() + 1;
    str->max = str->count;
    str->data = ohl::hooks::malloc<wchar_t>(str->count * sizeof(wchar_t));
    wcsncpy(str->data, value.c_str(), str->count - 1);
    str->data[str->count - 1] = '\0';
}

static FJsonValueString* create_json_string(std::wstring value) {
    auto obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
    obj->vf_table = vf_table.json_value_string;
    obj->type = EJson::String;
    alloc_string(&obj->str, value);

    return obj;
}

// clang-format off
static const uint32_t KNOWN_OBJECT_PATTERNS[3][16] = {{
    // Objects of size 1
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}, {
    // Objects of size 2
    0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000002, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}, {
    // Objects of size 3
    0x00000007, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000003, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}};
// clang-format on

static_assert(sizeof(*KNOWN_OBJECT_PATTERNS) == sizeof(FJsonObject::pattern));

template <size_t n>
static FJsonObject* create_json_object(
    std::array<std::pair<std::wstring, FJsonValue*>, n> entries) {
    static_assert(0 < n && n <= ARRAYSIZE(KNOWN_OBJECT_PATTERNS));

    auto obj = ohl::hooks::malloc<FJsonObject>(sizeof(FJsonObject));
    memcpy(obj->pattern, KNOWN_OBJECT_PATTERNS[n - 1], sizeof(obj->pattern));

    obj->entries.count = n;
    obj->entries.max = n;
    obj->entries.data = ohl::hooks::malloc<JSONObjectEntry>(n * sizeof(JSONObjectEntry));

    for (auto i = 0; i < n; i++) {
        obj->entries.data[i].hash_next_id = ((int32_t)i) - 1;

        alloc_string(&obj->entries.data[i].key, entries[i].first);

        obj->entries.data[i].value.obj = entries[i].second;
        add_ref_counter(&obj->entries.data[i].value, vf_table.shared_ptr_json_value);
    }

    return obj;
}

static FJsonValueObject* create_json_value_object(FJsonObject* obj) {
    auto val_obj = ohl::hooks::malloc<FJsonValueObject>(sizeof(FJsonValueObject));
    val_obj->vf_table = vf_table.json_value_object;
    val_obj->type = EJson::Object;

    val_obj->value.obj = obj;
    add_ref_counter(&val_obj->value, vf_table.shared_ptr_json_object);

    return val_obj;
}

void handle_discovery_from_json(FJsonObject** json) {
    gather_vf_tables(*json);

    auto services = (*json)->get<FJsonValueArray>(L"services");

    FJsonObject* micropatch = nullptr;
    for (auto i = 0; i < services->count(); i++) {
        auto service = services->get<FJsonValueObject>(i)->to_obj();
        if (service->get<FJsonValueString>(L"service_name")->to_wstr() == L"Micropatch") {
            micropatch = service;
            break;
        }
    }

    // This happens during the first verify call so don't throw
    if (micropatch == nullptr) {
        return;
    }

    if (!vf_table.found) {
        throw std::runtime_error("Didn't find vf tables in time!");
    }

    auto params = micropatch->get<FJsonValueArray>(L"parameters");

    auto new_hotfix_count = params->entries.count + INJECTED_HOTFIXES.size();
    if (new_hotfix_count > params->entries.max) {
        params->entries.max = new_hotfix_count;
        params->entries.data = ohl::hooks::realloc<TSharedPtr<FJsonValue>>(
            params->entries.data, params->entries.max * sizeof(TSharedPtr<FJsonValue>));
    }

    auto i = params->entries.count;
    for (const auto& [key, hotfix] : INJECTED_HOTFIXES) {
        auto hotfix_entry = create_json_object<2>(
            {{{L"key", create_json_string(key)}, {L"value", create_json_string(hotfix)}}});

        params->entries.data[i].obj = create_json_value_object(hotfix_entry);
        add_ref_counter(&params->entries.data[i], vf_table.shared_ptr_json_value);
        i++;
    }

    params->entries.count = new_hotfix_count;

    std::cout << "[OHL] Injected Hotfixes\n";
}

void handle_news_from_json(ohl::unreal::FJsonObject**) {}

}  // namespace ohl::processing
