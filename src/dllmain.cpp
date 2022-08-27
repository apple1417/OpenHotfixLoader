#include <pch.h>

#include "hooks.h"
#include "loader.h"

int startup_thread() {
    try {
        SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader");

        ohl::hooks::init();
        ohl::loader::init();
    } catch (std::exception ex) {
        std::cout << "[OHL] Exception occured during initalization: " << ex.what() << "\n";
    }

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
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
