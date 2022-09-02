#include <pch.h>

namespace ohl::util {

std::wstring widen(const std::string& str) {
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

}  // namespace ohl::util
