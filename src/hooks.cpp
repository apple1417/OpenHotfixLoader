#include <pch.h>

#include "unreal.h"
#include "processing.h"

using ohl::unreal::FJsonObject;

namespace ohl::hooks {

#pragma region API Hooking

typedef bool (*discovery_from_json)(void* this_service, FJsonObject** json);
static discovery_from_json original_discovery_from_json = nullptr;
bool detour_discovery_from_json(void* this_service, FJsonObject** json) {
    try {
        ohl::processing::handle_discovery_from_json(json);
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
