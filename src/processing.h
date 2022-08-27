#pragma once

#include "unreal.h"

namespace ohl::processing {

void handle_discovery_from_json(ohl::unreal::FJsonObject**);
void handle_news_from_json(ohl::unreal::FJsonObject**);

}  // namespace ohl::processing
