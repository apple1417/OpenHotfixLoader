#include <pch.h>

#include "hooks.h"
#include "loader.h"
#include "processing.h"
#include "logger.h"

static HMODULE this_module;
using namespace ohl::logger;

/**
 * @brief Main startup thread.
 * @note Instance of `ThreadProc`.
 *
 * @return unused.
 */
int startup_thread(void*) {
    try {
        SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader");

#ifdef DEBUG
        ohl::logger::logPrint( "[OHL] Running in debug mode" );
#endif

        ohl::hooks::init();
        ohl::processing::init();
        ohl::loader::init(this_module);
    } catch (std::exception ex) {
        ohl::logger::logPrint( std::stringstream() << "[OHL] Exception occured during initalization: " << ex.what() );
    }

    return 1;
}

/**
 * @brief Main entry point.
 *
 * @param hModule Handle to module for this dll.
 * @param ul_reason_for_call Reason this is being called.
 * @return True if loaded successfully, false otherwise.
 */
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
