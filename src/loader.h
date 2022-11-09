#pragma once

#include <pch.h>

namespace ohl::loader {

/**
 * @brief Struct representing a single hotfix entry.
 */
struct hotfix {
    std::string key;
    std::string value;

    hotfix(const std::string& key = "", const std::string& value = "") : key(key), value(value) {}

    bool operator==(const hotfix& rhs) const {
        return this->key == rhs.key && this->value == rhs.value;
    }
    bool operator!=(const hotfix& rhs) const { return !operator==(rhs); }
};

/**
 * @brief Struct representing a single injected news item. *
 */
struct news_item {
    std::string header;
    std::string image_url;
    std::string article_url;
    std::string body;

    news_item(const std::string& header = "",
              const std::string& image_url = "",
              const std::string& article_url = "",
              const std::string& body = "")
        : header(header), image_url(image_url), article_url(article_url), body(body) {}

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
