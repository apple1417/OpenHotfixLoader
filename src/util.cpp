#include <pch.h>

#include <doctest/doctest.h>

namespace ohl::util {
TEST_SUITE_BEGIN("utils");

std::string narrow(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

    auto num_chars =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
    char* str = reinterpret_cast<char*>(malloc((num_chars + 1) * sizeof(char)));
    if (!str) {
        throw std::runtime_error("Failed to convert utf16 string!");
    }

    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.size(), str, num_chars, NULL, NULL);
    str[num_chars] = L'\0';

    std::string ret{str};
    free(str);

    return ret;
}

std::wstring widen(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto num_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), NULL, 0);
    wchar_t* wstr = reinterpret_cast<wchar_t*>(malloc((num_chars + 1) * sizeof(wchar_t)));
    if (!wstr) {
        throw std::runtime_error("Failed to convert utf8 string!");
    }

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), wstr, num_chars);
    wstr[num_chars] = L'\0';

    std::wstring ret{wstr};
    free(wstr);

    return ret;
}

TEST_CASE("utils::narrow") {
    // Test using unicode literals directly to be safe of any encoding issues
    // UTF16 literals are `char16_t[]`s, which won't implicitly cast to wstring
    static_assert(sizeof(wchar_t) == sizeof(char16_t));

    CHECK(narrow((wchar_t*)u"test case") == u8"test case");
    CHECK(narrow((wchar_t*)u"υπόθεση δοκιμής") == u8"υπόθεση δοκιμής");
    CHECK(narrow((wchar_t*)u"прецедент") == u8"прецедент");
    CHECK(narrow((wchar_t*)u"テストケース") == u8"テストケース");
    CHECK(narrow((wchar_t*)u"\u0000\u007F\u0080\u1234") == u8"\u0000\u007F\u0080\u1234");

    CHECK(narrow((wchar_t*)u"test case") != u8"other string");
}

TEST_CASE("utils::widen") {
    static_assert(sizeof(wchar_t) == sizeof(char16_t));

    CHECK(widen(u8"test case") == (wchar_t*)u"test case");
    CHECK(widen(u8"υπόθεση δοκιμής") == (wchar_t*)u"υπόθεση δοκιμής");
    CHECK(widen(u8"прецедент") == (wchar_t*)u"прецедент");
    CHECK(widen(u8"テストケース") == (wchar_t*)u"テストケース");
    CHECK(widen(u8"\u0000\u007F\u0080\u1234") == (wchar_t*)u"\u0000\u007F\u0080\u1234");

    CHECK(widen(u8"test case") != (wchar_t*)u"other string");
}

TEST_CASE("utils::narrow - utils::widen round trip") {
    static_assert(sizeof(wchar_t) == sizeof(char16_t));

    CHECK(widen(narrow((wchar_t*)u"test case")) == (wchar_t*)u"test case");
    CHECK(widen(narrow((wchar_t*)u"υπόθεση δοκιμής")) == (wchar_t*)u"υπόθεση δοκιμής");
    CHECK(widen(narrow((wchar_t*)u"прецедент")) == (wchar_t*)u"прецедент");
    CHECK(widen(narrow((wchar_t*)u"テストケース")) == (wchar_t*)u"テストケース");
    CHECK(widen(narrow((wchar_t*)u"\u0000\u007F\u0080\u1234"))
          == (wchar_t*)u"\u0000\u007F\u0080\u1234");

    CHECK(widen(narrow((wchar_t*)u"test case")) != (wchar_t*)u"other string");

    CHECK(narrow(widen(u8"test case")) == u8"test case");
    CHECK(narrow(widen(u8"υπόθεση δοκιμής")) == u8"υπόθεση δοκιμής");
    CHECK(narrow(widen(u8"прецедент")) == u8"прецедент");
    CHECK(narrow(widen(u8"テストケース")) == u8"テストケース");
    CHECK(narrow(widen(u8"\u0000\u007F\u0080\u1234")) == u8"\u0000\u007F\u0080\u1234");

    CHECK(narrow(widen(u8"test case")) != u8"other string");
}

std::vector<std::filesystem::path> get_sorted_files_in_dir(const std::filesystem::path& path) {
    std::vector<std::filesystem::path> files{};
    for (const auto& dir_entry : std::filesystem::directory_iterator{path}) {
        if (dir_entry.is_directory()) {
            continue;
        }
        files.push_back(dir_entry.path());
    }
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) -> bool {
        return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
    });

    return files;
}

TEST_CASE("utils::get_sorted_files_in_dir") {
    const std::filesystem::path dir = std::filesystem::path("tests") / "sorted_files";
    const std::vector<std::filesystem::path> expected = {
        dir / "1.txt", dir / "5.txt", dir / "10.txt", dir / "a.txt", dir / "b.txt", dir / "c.txt",
    };

    CHECK(get_sorted_files_in_dir(dir) == expected);
}

std::string unescape_url(const std::string& url, bool extra_info) {
    DWORD len = (url.size() + 1) * sizeof(char);

    char* unescaped = reinterpret_cast<char*>(malloc(len));
    if (!unescaped) {
        throw std::runtime_error("Failed to unescape url!");
    }

    auto r = UrlUnescapeA(const_cast<char*>(url.c_str()), unescaped, &len,
                          (extra_info ? 0 : URL_DONT_UNESCAPE_EXTRA_INFO));

    std::string ret{unescaped};
    free(unescaped);

    return ret;
}

TEST_CASE("utils::unescape_url") {
    CHECK(unescape_url("https://example.com", false) == "https://example.com");
    CHECK(unescape_url("https://example.com#test", false) == "https://example.com#test");
    CHECK(unescape_url("https://exa%6Dple%2ecom", false) == "https://example.com");
    CHECK(unescape_url("https://exa%6Dple%2ecom?test", false) == "https://example.com?test");
    CHECK(unescape_url("https://exa%6Dple%2ecom#test", false) == "https://example.com#test");
    CHECK(unescape_url("https://exa%6Dple%2ecom#t%65st", false) == "https://example.com#t%65st");
    CHECK(unescape_url("https://exa%6Dple%2ecom#t%65st", true) == "https://example.com#test");
    CHECK(unescape_url("https://exa%6Dple%2ecom%23t%65st", true) == "https://example.com#test");
}

TEST_SUITE_END();
}  // namespace ohl::util
