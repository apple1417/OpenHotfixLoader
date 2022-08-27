#pragma once

#include <pch.h>

namespace ohl::loader {

void init(void);

std::vector<std::pair<std::wstring, std::wstring>> get_hotfixes(void);
std::wstring get_news_header(void);
std::wstring get_news_body(void);

}  // namespace ohl::loader
