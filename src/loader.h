#pragma once

#include <pch.h>

namespace ohl::loader {

/**
 * @brief Struct representing a single hotfix entry.
 */
struct hotfix {
    std::wstring key;
    std::wstring value;

    bool operator==(const hotfix& rhs) const {
        return this->key == rhs.key && this->value == rhs.value;
    }
    bool operator!=(const hotfix& rhs) const { return !operator==(rhs); }
};

/**
 * @brief Struct representing a single injected news item. *
 */
struct news_item {
    std::wstring header;
    std::wstring image_url;
    std::wstring article_url;
    std::wstring body;

    bool operator==(const news_item& rhs) const {
        return this->header == rhs.header && this->image_url == rhs.image_url
               && this->article_url == rhs.article_url && this->body == rhs.body;
    }
    bool operator!=(const news_item& rhs) const { return !operator==(rhs); }
};

/**
 * @brief Initalizes the loader module.
 */
void init(void);

/**
 * @brief Starts reloads the hotfix list.
 * @note Runs in a thread, `get_hotfixes` or `get_news_items` calls will block until it compeltes.
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
