#pragma once

#include <pch.h>

namespace ohl::loader {

/**
 * @brief Struct representing a single hotfix entry.
 */
struct hotfix {
    std::wstring type;
    std::wstring value;
};

/**
 * @brief Struct representing a single injected news item. *
 */
struct news_item {
    std::wstring header;
    std::wstring url;
    std::wstring body;
};

/**
 * @brief Initalizes the loader module.
 */
void init(void);

/**
 * @brief Reloads the hotfix list.
 */
void reload(void);

/**
 * @brief Get the list of hotfixes to inject.
 *
 * @return A list of hotfixes.
 */
std::deque<hotfix> get_hotfixes(void);

/**
 * @brief Get the list of news items to inject.
 *
 * @return A list of news items.
 */
std::deque<news_item> get_news_items(void);

}  // namespace ohl::loader
