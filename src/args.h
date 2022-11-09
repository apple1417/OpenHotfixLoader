#pragma once

#include <pch.h>

namespace ohl::args {

/**
 * @brief Parses all command line args, and otherwise initalizes the args module.
 *
 * @param this_module Handle to this dll's module.
 */
void init(HMODULE this_module);

/**
 * @brief Checks if debug mode is on.
 *
 * @return True if debug mode was specified, false otherwise.
 */
bool debug(void);

/**
 * @brief Checks if to dump hotfixes.
 *
 * @return True if to dump hotfixes, false otherwise.
 */
bool dump_hotfixes(void);

/**
 * @brief Gets the path to the current exe.
 *
 * @return The path to the current exe, or an empty string if not found.
 */
std::filesystem::path exe_path(void);

/**
 * @brief Gets the path to the current dll.
 *
 * @return The path to the current dll, or an empty string if not found.
 */
std::filesystem::path dll_path(void);

}  // namespace ohl::args
