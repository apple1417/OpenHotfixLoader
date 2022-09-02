#pragma once

#include <pch.h>

using hotfix_list = std::deque<std::pair<std::wstring, std::wstring>>;

namespace ohl::loader {

/**
 * @brief Initalizes the loader module.
 */
void init(void);

/**
 * @brief Reloads the hotfix list.
 */
void reload_hotfixes(void);

/**
 * @brief Get the list of hotfixes to inject.
 *
 * @return A list of hotfixes.
 */
hotfix_list get_hotfixes(void);

/**
 * @brief Get the header string to inject into the news.
 *
 * @return The news header.
 */
std::wstring get_news_header(void);

/**
 * @brief Get a string of loaded mods.
 * @note Intended to be injected into the news body, and to be used for debug logging.
 *
 * @return A string containing all loaded mod files.
 */
std::wstring get_loaded_mods_str(void);

}  // namespace ohl::loader
