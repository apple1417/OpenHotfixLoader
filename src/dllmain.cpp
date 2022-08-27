#include <pch.h>

#include "hooks.h"
#include "loader.h"
#include "processing.h"

static HMODULE this_module;

int startup_thread() {
    try {
        SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader");

        ohl::hooks::init();
        ohl::processing::init();
        ohl::loader::init(this_module);
    } catch (std::exception ex) {
        std::cout << "[OHL] Exception occured during initalization: " << ex.what() << "\n";
    }

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            this_module = hModule;
            DisableThreadLibraryCalls(hModule);
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)startup_thread, NULL, NULL, NULL);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
