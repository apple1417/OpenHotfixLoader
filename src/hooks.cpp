#include <pch.h>

#include "unreal.h"

using namespace ohl::unreal;

namespace ohl::hooks {

#pragma region Handlers

static void handle_discovery(FJsonObject** json) {
    auto services = (*json)->get<FJsonValueArray>(L"services");

    FJsonObject* micropatch = nullptr;

    for (auto i = 0; i < services->count(); i++) {
        auto service = services->get<FJsonValueObject>(i)->to_obj();
        auto service_name = service->get<FJsonValueString>(L"service_name")->to_wstr();
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

    auto params = micropatch->get<FJsonValueArray>(L"parameters");
    for (auto i = 0; i < params->count(); i++) {
        auto hotfix = params->get<FJsonValueObject>(i)->to_obj();
        auto key = hotfix->get<FJsonValueString>(L"key")->to_str();
        auto value = hotfix->get<FJsonValueString>(L"value")->to_str();

        std::cout << "[OHL] " << key << "," << value << "\n";
    }
}

#pragma endregion

#pragma region API Hooking

typedef bool (*discovery_from_json)(void* this_service, FJsonObject** json);
static discovery_from_json original_discovery_from_json = nullptr;
bool detour_discovery_from_json(void* this_service, FJsonObject** json) {
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
