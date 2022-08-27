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

static const uint32_t KNOWN_ONE_PATTERN[16] = {
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
};

static const uint32_t KNOWN_TWO_PATTERN[16] = {
    0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000002, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
};

static const uint32_t KNOWN_THREE_PATTERN[16] {
    0x00000007, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000003, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
};

static_assert(sizeof(KNOWN_TWO_PATTERN) == sizeof(FJsonObject::pattern));

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

static void alloc_string(FString* str, std::wstring value) {
    str->count = value.size() + 1;
    str->max = str->count;
    str->data = ohl::hooks::malloc<wchar_t>(str->count * sizeof(wchar_t));
    wcsncpy(str->data, value.c_str(), str->count - 1);
    str->data[str->count - 1] = '\0';
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
        auto val_obj = ohl::hooks::malloc<FJsonValueObject>(sizeof(FJsonValueObject));
        val_obj->vf_table = vf_table.json_value_object;
        val_obj->type = EJson::Object;

        params->entries.data[i].obj = val_obj;
        add_ref_counter(&params->entries.data[i], vf_table.shared_ptr_json_value);
        i++;

        auto main_obj = ohl::hooks::malloc<FJsonObject>(sizeof(FJsonObject));
        memcpy(main_obj->pattern, KNOWN_TWO_PATTERN, sizeof(KNOWN_TWO_PATTERN));
        main_obj->entries.count = 2;
        main_obj->entries.max = main_obj->entries.count;
        main_obj->entries.data =
            ohl::hooks::malloc<JSONObjectEntry>(main_obj->entries.count * sizeof(JSONObjectEntry));

        val_obj->value.obj = main_obj;
        add_ref_counter(&val_obj->value, vf_table.shared_ptr_json_object);

        auto key_obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
        key_obj->vf_table = vf_table.json_value_string;
        key_obj->type = EJson::String;
        alloc_string(&key_obj->str, key);

        alloc_string(&main_obj->entries.data[0].key, L"key");
        main_obj->entries.data[0].value.obj = key_obj;
        add_ref_counter(&main_obj->entries.data[0].value, vf_table.shared_ptr_json_value);
        main_obj->entries.data[0].hash_next_id = -1;

        auto hotfix_obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
        hotfix_obj->vf_table = vf_table.json_value_string;
        hotfix_obj->type = EJson::String;
        alloc_string(&hotfix_obj->str, hotfix);

        alloc_string(&main_obj->entries.data[1].key, L"value");
        main_obj->entries.data[1].value.obj = hotfix_obj;
        add_ref_counter(&main_obj->entries.data[1].value, vf_table.shared_ptr_json_value);
        main_obj->entries.data[0].hash_next_id = 0;
    }

    params->entries.count = new_hotfix_count;

    std::cout << "[OHL] Injected Hotfixes\n";
}

void handle_news_from_json(ohl::unreal::FJsonObject**) {
    
}
}  // namespace ohl::processing
