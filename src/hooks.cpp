#include <pch.h>

namespace ohl::hooks {

typedef bool (*discovery_from_json)(void* this_service, void* json);
static discovery_from_json original_discovery_from_json = nullptr;
bool detour_discovery_from_json(void* this_service, void* json) {
    std::cout << "[OHL] discovery called";
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

}  // namespace ohl::hooks
