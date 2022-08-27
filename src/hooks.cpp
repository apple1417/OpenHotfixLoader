#include <pch.h>

#include "unreal.h"
#include "processing.h"

using ohl::unreal::FJsonObject;

namespace ohl::hooks {

#pragma region Types

typedef void* (*fmemory_malloc)(size_t count, uint32_t align);
typedef void* (*fmemory_realloc)(void* original, size_t count, uint32_t align);
typedef void (*fmemory_free)(void* data);
typedef bool (*discovery_from_json)(void* this_service, FJsonObject** json);
typedef bool (*news_from_json)(void* this_response, FJsonObject** json);

#pragma endregion

#pragma region Sig Scanning

// TODO: actually scan

static discovery_from_json* get_discovery_from_json(void) {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<discovery_from_json*>(base_addr + 0x2B0F300);
}

static news_from_json * get_news_from_json(void) {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<discovery_from_json*>(base_addr + 0x2B224B0);
}

static fmemory_malloc get_fememory_malloc() {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<fmemory_malloc>(base_addr + 0x15DD5D0);
}

static fmemory_realloc get_fememory_realloc() {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<fmemory_realloc>(base_addr + 0x15DEB00);
}

static fmemory_free get_fememory_free() {
    auto base_addr = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    return reinterpret_cast<fmemory_free>(base_addr + 0x15D37B0);
}

#pragma endregion

#pragma region Detours

static discovery_from_json original_discovery_from_json = nullptr;
bool detour_discovery_from_json(void* this_service, FJsonObject** json) {
    try {
        ohl::processing::handle_discovery_from_json(json);
    } catch (std::exception ex) {
        std::cout << "[OHL] Exception occured in discovery hook: " << ex.what() << "\n";
    }

    return original_discovery_from_json(this_service, json);
}

static news_from_json original_news_from_json = nullptr;
bool detour_news_from_json(void* this_service, FJsonObject** json) {
    try {
        ohl::processing::handle_news_from_json(json);
    } catch (std::exception ex) {
        std::cout << "[OHL] Exception occured in news hook: " << ex.what() << "\n";
    }

    return original_news_from_json(this_service, json);
}

#pragma endregion

static fmemory_malloc pointer_malloc = nullptr;
static fmemory_realloc pointer_realloc = nullptr;
static fmemory_free pointer_free = nullptr;

void init(void) {
    pointer_malloc = get_fememory_malloc();
    pointer_realloc = get_fememory_realloc();
    pointer_free = get_fememory_free();

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

    auto pointer_news_from_json = get_news_from_json();
    ret = MH_CreateHook(pointer_news_from_json, &detour_news_from_json,
                        reinterpret_cast<LPVOID*>(&original_news_from_json));
    if (ret != MH_OK) {
        std::cout << "[OHL] MH_CreateHook failed " << ret << "\n";
        return;
    }

    ret = MH_EnableHook(pointer_news_from_json);
    if (ret != MH_OK) {
        std::cout << "[OHL] MH_EnableHook failed " << ret << "\n";
        return;
    }

    std::cout << "[OHL] Hooks injected successfully\n";
}

void* malloc_raw(size_t count) {
    if (pointer_malloc == nullptr) {
        throw std::runtime_error("Tried to call malloc, which was not found!");
    }
    auto ret = pointer_malloc(count, 8);
    if (ret == nullptr) {
        throw std::runtime_error("Failed to allocate memory!");
    }
    memset(ret, 0, count);
    return ret;
}

void* realloc_raw(void* original, size_t count) {
    if (pointer_realloc == nullptr) {
        throw std::runtime_error("Tried to call realloc, which was not found!");
    }
    auto ret = pointer_realloc(original, count, 8);
    if (ret == nullptr) {
        throw std::runtime_error("Failed to re-allocate memory!");
    }
    return ret;
}

void free(void* data) {
    if (pointer_free == nullptr) {
        throw std::runtime_error("Tried to call free, which was not found!");
    }
    pointer_free(data);
}

}  // namespace ohl::hooks
