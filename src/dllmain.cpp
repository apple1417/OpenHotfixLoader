#include <pch.h>

#include "hooks.h"
#include "loader.h"
#include "processing.h"

static const std::string LOG_FILE_NAME = "OpenHotfixLoader.log";

static HMODULE this_module;

std::filesystem::path exe_path = "";
std::filesystem::path dll_path = "";

/**
 * @brief Main startup thread.
 * @note Instance of `ThreadProc`.
 *
 * @return unused.
 */
static int32_t startup_thread(void*) {
    try {
        SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader");

        wchar_t buf[FILENAME_MAX];
        if (GetModuleFileName(NULL, buf, sizeof(buf))) {
            exe_path = std::filesystem::path(std::wstring(buf));
        }
        if (GetModuleFileName(this_module, buf, sizeof(buf))) {
            dll_path = std::filesystem::path(std::wstring(buf));
        }

        static plog::ConsoleAppender<plog::MessageOnlyFormatter> consoleAppender;
        plog::init(plog::debug, dll_path.replace_filename(LOG_FILE_NAME).c_str())
            .addAppender(&consoleAppender);

#ifdef DEBUG
        LOGD << "[OHL] Running in debug mode";
#endif

        ohl::hooks::init();
        ohl::processing::init();
        ohl::loader::init();

#ifdef DEBUG
        ohl::loader::reload();
#endif
    } catch (std::exception ex) {
        LOGF << "[OHL] Exception occured during initalization: " << ex.what() << "\n";
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
