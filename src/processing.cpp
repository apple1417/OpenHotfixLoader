#include <pch.h>

#include "hooks.h"
#include "unreal.h"


using namespace ohl::unreal;

namespace ohl::processing {

static std::vector<std::pair<std::wstring, std::wstring>> INJECTED_HOTFIXES = {
    { L"SparkPatchEntry999999", L"(1,1,0,),/Game/PlayerCharacters/_Shared/_Design/Sliding/ControlledMove_Global_Sliding.Default__ControlledMove_Global_Sliding_C,Duration.BaseValueConstant,0,,5000" },
};


static void alloc_string(FString* str, std::wstring value) {
    str->count = value.size() + 1;
    str->max =  str->count;
    str->data = ohl::hooks::malloc<wchar_t>(str->count * sizeof(wchar_t));
    wcsncpy(str->data, value.c_str(), str->count - 1);
    str->data[str->count - 1] = '\0';
}


template <typename T>
static void add_ref_counter(TSharedPtr<T>* ptr, void* vf_table) {
    ptr->ref_controller = ohl::hooks::malloc<FReferenceControllerBase>(sizeof(FReferenceControllerBase));
    ptr->ref_controller->vf_table = vf_table;
    ptr->ref_controller->ref_count = 1;
    ptr->ref_controller->weak_ref_count = 1;
    ptr->ref_controller->obj = ptr->obj;
}

void handle_discovery_from_json(ohl::unreal::FJsonObject** json) {
    auto services = (*json)->get<FJsonValueArray>(L"services");

    FJsonObject* micropatch = nullptr;

    void* json_str_vf_table = nullptr;
    void* ref_json_obj_vf_table = nullptr;

    for (auto i = 0; i < services->count(); i++) {
        auto service = services->get<FJsonValueObject>(i);
        auto service_obj = service->to_obj();
        auto service_name = service_obj->get<FJsonValueString>(L"service_name");
        if (service_name->to_wstr() == L"Micropatch") {
            micropatch = service_obj;

            json_str_vf_table = service_name->vf_table;
            ref_json_obj_vf_table = service->vf_table;
            break;
        }
    }

    // This happens during the first verify call so don't throw
    if (micropatch == nullptr) {
        return;
    }

    std::cout << "[OHL] Found Hotfixes\n";
    auto params = micropatch->get<FJsonValueArray>(L"parameters");

    void* json_obj_vf_table = params->entries.data[0].obj->vf_table;
    void* ref_json_val_vf_table = params->entries.data[0].ref_controller->vf_table;

    auto new_hotfix_count = params->entries.count + INJECTED_HOTFIXES.size();

    if (new_hotfix_count > params->entries.max) {
        params->entries.data = ohl::hooks::realloc<TSharedPtr<FJsonValue>>(params->entries.data, new_hotfix_count * sizeof(TSharedPtr<FJsonValue>));
    }

    auto i = params->entries.count;
    for (const auto& [key, hotfix] : INJECTED_HOTFIXES) {
        auto val_obj = ohl::hooks::malloc<FJsonValueObject>(sizeof(FJsonValueObject));
        val_obj->vf_table = json_obj_vf_table;
        val_obj->type = EJson::Object;

        params->entries.data[i].obj = val_obj;
        add_ref_counter(&params->entries.data[i], ref_json_val_vf_table);
        i++;

        auto main_obj = ohl::hooks::malloc<FJsonObject>(sizeof(FJsonObject));
        memcpy(main_obj->pattern, FJsonObject::KNOWN_PATTERN, sizeof(FJsonObject::KNOWN_PATTERN));
        main_obj->entries.count = 2;
        main_obj->entries.max = main_obj->entries.count;
        main_obj->entries.data = ohl::hooks::malloc<JSONObjectEntry>(main_obj->entries.count * sizeof(JSONObjectEntry));

        val_obj->value.obj = main_obj;
        add_ref_counter(&val_obj->value, ref_json_obj_vf_table);

        auto key_obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
        key_obj->vf_table = json_str_vf_table;
        key_obj->type = EJson::String;
        alloc_string(&key_obj->str, key);

        alloc_string(&main_obj->entries.data[0].key, L"key");
        main_obj->entries.data[0].value.obj = key_obj;
        add_ref_counter(&main_obj->entries.data[0].value, ref_json_val_vf_table);
        main_obj->entries.data[0].hash_next_id = -1;

        auto hotfix_obj = ohl::hooks::malloc<FJsonValueString>(sizeof(FJsonValueString));
        hotfix_obj->vf_table = json_str_vf_table;
        hotfix_obj->type = EJson::String;
        alloc_string(&hotfix_obj->str, hotfix);

        alloc_string(&main_obj->entries.data[1].key, L"value");
        main_obj->entries.data[1].value.obj = hotfix_obj;
        add_ref_counter(&main_obj->entries.data[1].value, ref_json_val_vf_table);
    }

    params->entries.count = new_hotfix_count;
    params->entries.max = new_hotfix_count;
}

}  // namespace ohl::processing
