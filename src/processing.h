#pragma once

#include "unreal.h"

namespace ohl::processing {

/**
 * @brief Initalizes the hook processing module.
 */
void init(void);

/**
 * @brief Handles `GbxSparkSdk::Discovery::Api::GetServicesVerification` calls, using them to start
 *        reloading hotfixes.
 */
void handle_get_verification(void);

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

/**
 * @brief Handles `FOnlineImageManager::AddImageToFileCache` calls, checking if to block execution
 *        to prevent injected images from being cached.
 *
 * @param req The spark request that downloaded this image.
 * @return True if the function is allowed to continue, false if to block execution.
 */
bool handle_add_image_to_cache(ohl::unreal::TSharedPtr<ohl::unreal::FSparkRequest>* req);

}  // namespace ohl::processing
