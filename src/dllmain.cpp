#include <pch.h>

#include "args.h"
#include "hooks.h"
#include "loader.h"
#include "processing.h"
#include "version.h"

static const std::string LOG_FILE_NAME = "OpenHotfixLoader.log";

static HMODULE this_module;

/**
 * @brief Main startup thread.
 * @note Instance of `ThreadProc`.
 *
 * @return unused.
 */
static int32_t startup_thread(void*) {
    try {
        SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader");

        ohl::args::init(this_module);

        static plog::ConsoleAppender<plog::MessageOnlyFormatter> consoleAppender;
        plog::init(ohl::args::debug() ? plog::debug : plog::info,
                   ohl::args::dll_path().replace_filename(LOG_FILE_NAME).c_str())
            .addAppender(&consoleAppender);

        LOGI << "[OHL] Launched " VERSION_STRING;
#ifdef DEBUG
        LOGD << "[OHL] Running debug build";
#endif

        ohl::hooks::init();
        ohl::loader::init();

#ifdef DEBUG
        ohl::loader::reload();
#endif
    } catch (std::exception ex) {
        LOGF << "[OHL] Exception occured during initalization: " << ex.what();
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
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startup_thread, NULL, 0, NULL);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
