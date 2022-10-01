#include <pch.h>

namespace ohl::util {

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

std::wstring unescape_url(const std::wstring& url, bool extra_info) {
    wchar_t* unescaped = reinterpret_cast<wchar_t*>(malloc((url.size() + 1) * sizeof(wchar_t)));

    DWORD len = url.size() * 2;
    UrlUnescapeW(const_cast<wchar_t*>(url.c_str()), unescaped, &len,
                 URL_UNESCAPE_AS_UTF8 | (extra_info ? 0 : URL_DONT_UNESCAPE_EXTRA_INFO));

    std::wstring ret{unescaped};
    free(unescaped);

    return ret;
}

}  // namespace ohl::util
