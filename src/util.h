#include <pch.h>

namespace ohl::util {

/**
 * @brief Narrows a utf-16 wstring to a utf-8 string.
 *
 * @param str The input wstring.
 * @return The output string.
 */
std::string narrow(const std::wstring& wstr);

/**
 * @brief Widens a utf-8 string to a utf-16 wstring.
 *
 * @param str The input string.
 * @return The output wstring.
 */
std::wstring widen(const std::string& str);

/**
 * @brief Get all files in a directory, sorted numerically.
 * @note Returns 1, 5, 10, etc.
 *
 * @param path Path to the directory.
 * @return A list of file paths.
 */
std::vector<std::filesystem::path> get_sorted_files_in_dir(const std::filesystem::path& path);

}  // namespace ohl::util
