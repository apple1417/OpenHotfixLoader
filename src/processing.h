#pragma once

#include "unreal.h"

namespace ohl::processing {

/**
 * @brief Initalizes the hook processing module.
 */
void init(void);

/**
 * @brief Handles `GbxSparkSdk::Discovery::Services::FromJson` calls, inserting our custom hotfixes.
 *
 * @param json Unreal json objects containing the received data.
 */
void handle_discovery_from_json(ohl::unreal::FJsonObject** json);

/**
 * @brief Handles `GbxSparkSdk::News::NewsResponse::FromJson` calls, inserting our custom article.
 *
 * @param json Unreal json objects containing the received data.
 */
void handle_news_from_json(ohl::unreal::FJsonObject** json);

}  // namespace ohl::processing
