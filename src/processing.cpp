#include <pch.h>

#include "hooks.h"
#include "loader.h"
#include "unreal.h"

using namespace ohl::unreal;

namespace ohl::processing {

static const auto HOTFIX_COUNTER_OFFSET = 100000;
static const std::wstring NEWS_ICON =
    L"https://raw.githubusercontent.com/apple1417/OpenHotfixLoader/master/news_icon.png";
static const std::wstring HOTFIX_DUMP_FILE = L"hotfixes.dump";

static bool dump_hotfixes = false;

/**
 * @brief Struct holding all the vf tables we need to grab copies of.
 */
struct vf_tables {
    bool found;
    void* json_value_string;
    void* json_value_array;
    void* json_value_object;
    void* shared_ptr_json_object;
    void* shared_ptr_json_value;
};

static vf_tables vf_table = {};

// Haven't managed to fully reverse engineer json objects, but their extra data is always constant
//  for objects with the same amount of entries, so as a hack we can just hardcode it

// clang-format off
static const uint32_t KNOWN_OBJECT_PATTERNS[3][16] = {{
    // Objects of size 1
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
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

/**
 * @brief Gathers all required vf table pointers and fills in the vf table struct.
 *
 * @param discovery_json The json object received by the discovery hook.
 */
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

/**
 * @brief Adds a reference controller to a shared pointer.
 * @note The object must already be set before calling this.
 *
 * @tparam T The type of the shared pointer.
 * @param ptr The shared pointer to edit.
 * @param vf_table The vf table for the shared pointer's type.
 */
template <typename T>
static void add_ref_controller(TSharedPtr<T>* ptr, void* vf_table) {
    ptr->ref_controller =
        ohl::hooks::malloc<FReferenceControllerBase>(sizeof(FReferenceControllerBase));
    ptr->ref_controller->vf_table = vf_table;
    ptr->ref_controller->ref_count = 1;
    ptr->ref_controller->weak_ref_count = 1;
    ptr->ref_controller->obj = ptr->obj;
}

/**
 * @brief Allocated memory to set an FString to a given value.
 * @note Also sets the string count.
 *
 * @param str The FString to fill.
 * @param value The value to set.
 */
static void alloc_string(FString* str, std::wstring value) {
    str->count = value.size() + 1;
    str->max = str->count;
    str->data = ohl::hooks::malloc<wchar_t>(str->count * sizeof(wchar_t));
    wcsncpy(str->data, value.c_str(), str->count - 1);
    str->data[str->count - 1] = '\0';
}

/**
 * @brief Creates a json string object.
 *
 * @param value The value of the string.
 * @return A pointer to the new object.
 */
static FJsonValueString* create_json_string(std::wstring value) {
    auto obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
    obj->vf_table = vf_table.json_value_string;
    obj->type = EJson::String;
    alloc_string(&obj->str, value);

    return obj;
}

/**
 * @brief Creates a json object.
 *
 * @tparam n The amount of entries in the object.
 * @param entries Key-value pairs of the object's entries.
 * @return A pointer to the new object.
 */
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
        add_ref_controller(&obj->entries.data[i].value, vf_table.shared_ptr_json_value);
    }

    return obj;
}

/**
 * @brief Create a json array object.
 *
 * @param entries The entries in the array.
 * @return A pointer to the new object
 */
static FJsonValueArray* create_json_array(std::vector<FJsonValue*> entries) {
    auto obj = ohl::hooks::malloc<FJsonValueArray>(sizeof(FJsonValueArray));
    obj->vf_table = vf_table.json_value_array;
    obj->type = EJson::Array;

    const auto n = entries.size();

    obj->entries.count = n;
    obj->entries.max = n;
    obj->entries.data =
        ohl::hooks::malloc<TSharedPtr<FJsonValue>>(n * sizeof(TSharedPtr<FJsonValue>));

    for (auto i = 0; i < n; i++) {
        obj->entries.data[i].obj = entries[i];
        add_ref_controller(&obj->entries.data[i], vf_table.shared_ptr_json_value);
    }

    return obj;
}

/**
 * @brief Create a json value object from a raw object.
 *
 * @param obj The object to create a value object of.
 * @return A pointer to the new value object.
 */
static FJsonValueObject* create_json_value_object(FJsonObject* obj) {
    auto val_obj = ohl::hooks::malloc<FJsonValueObject>(sizeof(FJsonValueObject));
    val_obj->vf_table = vf_table.json_value_object;
    val_obj->type = EJson::Object;

    val_obj->value.obj = obj;
    add_ref_controller(&val_obj->value, vf_table.shared_ptr_json_object);

    return val_obj;
}

/**
 * @brief Gets the current time in an iso8601-formatted string.
 *
 * @return The current time string.
 */
static std::wstring get_current_time_str(void) {
    auto time = std::time(nullptr);
    auto utc = std::gmtime(&time);

    // Only need 25 but let's be safe
    wchar_t buf[64] = {};
    wcsftime(buf, sizeof(buf), L"%FT%T.000Z", utc);

    return std::wstring(buf);
}

void init(void) {
    dump_hotfixes = std::wstring(GetCommandLine()).find(L"--dump-hotfixes") != std::string::npos;
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

    ohl::loader::reload_hotfixes();
    auto hotfixes = ohl::loader::get_hotfixes();

    auto params = micropatch->get<FJsonValueArray>(L"parameters");
    auto new_hotfix_count = params->entries.count + hotfixes.size();
    if (new_hotfix_count > params->entries.max) {
        params->entries.max = new_hotfix_count;
        params->entries.data = ohl::hooks::realloc<TSharedPtr<FJsonValue>>(
            params->entries.data, params->entries.max * sizeof(TSharedPtr<FJsonValue>));
    }

    auto i = params->entries.count;
    for (const auto& [key, hotfix] : hotfixes) {
        auto hotfix_entry = create_json_object<2>(
            {{{L"key", create_json_string(key + std::to_wstring(i + HOTFIX_COUNTER_OFFSET))},
              {L"value", create_json_string(hotfix)}}});

        params->entries.data[i].obj = create_json_value_object(hotfix_entry);
        add_ref_controller(&params->entries.data[i], vf_table.shared_ptr_json_value);
        i++;
    }

    params->entries.count = new_hotfix_count;

    std::cout << "[OHL] Injected hotfixes\n";

    if (dump_hotfixes) {
        std::wofstream dump{HOTFIX_DUMP_FILE};
        dump.imbue(
            std::locale(std::locale::empty(), new std::codecvt<char16_t, char, std::mbstate_t>));
        for (auto i = 0; i < params->count(); i++) {
            auto entry = params->get<FJsonValueObject>(i)->to_obj();
            auto key = entry->get<FJsonValueString>(L"key")->to_wstr();
            auto value = entry->get<FJsonValueString>(L"value")->to_wstr();

            dump << key << L": " << value << "\n";
        }
        dump.close();
        std::cout << "[OHL] Dumped hotfixes to file\n";
    }
}

void handle_news_from_json(ohl::unreal::FJsonObject** json) {
    if (!vf_table.found) {
        throw std::runtime_error("Didn't find vf tables in time!");
    }

    auto news_data = (*json)->get<FJsonValueArray>(L"data");

    if (news_data->entries.count + 1 > news_data->entries.max) {
        news_data->entries.max = news_data->entries.count + 1;
        news_data->entries.data = ohl::hooks::realloc<TSharedPtr<FJsonValue>>(
            news_data->entries.data, news_data->entries.max * sizeof(TSharedPtr<FJsonValue>));
    }

    auto contents_obj = create_json_object<2>(
        {{{L"header", create_json_string(ohl::loader::get_news_header())},
          {L"body", create_json_string(ohl::loader::get_loaded_mods_str())}}});
    auto contents_arr = create_json_array({create_json_value_object(contents_obj)});

    auto meta_tag_obj =
        create_json_object<1>({{{L"tag", create_json_string(L"img_game_sm_noloc")}}});
    auto tags_obj = create_json_object<2>({{{L"meta_tag", create_json_value_object(meta_tag_obj)},
                                            {L"value", create_json_string(NEWS_ICON)}}});
    auto tags_arr = create_json_array({create_json_value_object(tags_obj)});

    auto availablities_obj =
        create_json_object<1>({{{L"startTime", create_json_string(get_current_time_str())}}});
    auto availabilities_arr = create_json_array({create_json_value_object(availablities_obj)});

    auto news_obj = create_json_object<3>({{{L"contents", contents_arr},
                                            {L"article_tags", tags_arr},
                                            {L"availabilities", availabilities_arr}}});

    // Shift the existing entries so that ours is at the front
    memmove(&news_data->entries.data[1], &news_data->entries.data[0],
            news_data->entries.count * sizeof(TSharedPtr<FJsonValue>));

    news_data->entries.data[0].obj = create_json_value_object(news_obj);
    add_ref_controller(&news_data->entries.data[0], vf_table.shared_ptr_json_value);
    news_data->entries.count++;

    std::cout << "[OHL] Injected news\n";
}

}  // namespace ohl::processing
