#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <MinHook.h>
#include <shlwapi.h>

#ifdef __cplusplus
#include <cpr/cpr.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>

#include <array>
#include <codecvt>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;
#endif

#ifdef __MINGW32__
// blank out SetThreadDescription
#define SetThreadDescription(x, y)
#define URL_UNESCAPE_AS_UTF8 0x00040000
#endif
