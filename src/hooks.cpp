#include <pch.h>

#include "jobject.h"

using namespace ohl::jobject;

namespace ohl::hooks {

#pragma region Handlers

static void handle_discovery(TArray<JDictEntry>** json) {
    auto main_entry = (*json)->data[0];
    auto main_key = main_entry.key_str();
    if (main_key != "services") {
        throw std::runtime_error("Outermost key is invalid: " + main_key);
    }
    auto services = main_entry.value->cast<JArray>();

    JDict* micropatch = nullptr;

    for (auto i = 0; i < services->count(); i++) {
        auto service = services->get<JDict>(i);
        auto service_name = service->get<JString>(L"service_name")->to_wstr();
        if (service_name == L"Micropatch") {
            micropatch = service;
            break;
        }
    }

    // This happens during the first verify call so don't throw
    if (micropatch == nullptr) {
        return;
    }

    std::cout << "[OHL] Found Hotfixes:\n";

    auto params = micropatch->get<JArray>(L"parameters");
    for (auto i = 0; i < params->count(); i++) {
        auto hotfix = params->get<JDict>(i);
        auto key = hotfix->get<JString>(L"key")->to_str();
        auto value = hotfix->get<JString>(L"value")->to_str();

        std::cout << "[OHL] " << key << "," << value << "\n";
    }
}

#pragma endregion

#pragma region API Hooking

typedef bool (*discovery_from_json)(void* this_service, TArray<JDictEntry>** json);
static discovery_from_json original_discovery_from_json = nullptr;
bool detour_discovery_from_json(void* this_service, TArray<JDictEntry>** json) {
    try {
        handle_discovery(json);
    } catch (std::exception ex) {
        std::cout << "[OHL] Exception occured in hook: " << ex.what() << "\n";
    }

    return original_discovery_from_json(this_service, json);
}

static discovery_from_json* get_discovery_from_json(void) {
    // TODO: sigscan
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<discovery_from_json*>(base_addr + 0x2B0F300);
}

void init(void) {
    auto ret = MH_Initialize();
    if (ret != MH_OK) {
        std::cout << "[OHL] MH_Initialize failed " << ret << "\n";
        return;
    }

    auto pointer_discovery_from_json = get_discovery_from_json();

    ret = MH_CreateHook(pointer_discovery_from_json, &detour_discovery_from_json,
                        reinterpret_cast<LPVOID*>(&original_discovery_from_json));
    if (ret != MH_OK) {
        std::cout << "[OHL] MH_CreateHook failed " << ret << "\n";
        return;
    }

    ret = MH_EnableHook(pointer_discovery_from_json);
    if (ret != MH_OK) {
        std::cout << "[OHL] MH_EnableHook failed " << ret << "\n";
        return;
    }

    std::cout << "[OHL] Hooks injected successfully\n";
}

#pragma endregion

}  // namespace ohl::hooks
