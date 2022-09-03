#include <pch.h>

#include "dllmain.h"
#include "loader.h"
#include "util.h"
#include "version.h"

namespace ohl::loader {

static const std::wstring WHITESPACE = L" \f\n\r\t\b";

static const std::wstring HOTFIX_COMMAND = L"spark";
static const std::wstring NEWS_COMMAND = L"injectnewsitem";
static const std::wstring EXEC_COMMAND = L"exec";
static const std::wstring URL_COMMAND = L"url=";

static const std::wstring TYPE_11_PREFIX = L"sparkearlylevelpatchentry,(1,11,0,";
static const std::wstring TYPE_11_DELAY_TYPE = L"SparkEarlyLevelPatchEntry";
static const std::wstring TYPE_11_DELAY_VALUE =
    L"(1,1,0,{map}),/Game/Pickups/Ammo/"
    L"BPAmmoItem_Pistol.Default__BPAmmoItem_Pistol_C,ItemMeshComponent.Object..StaticMesh,0,,"
    L"StaticMesh'\"{mesh}\"'";
static const std::vector<std::wstring> TYPE_11_DELAY_MESHES = {
    L"/Engine/EditorMeshes/Camera/SM_CraneRig_Arm.SM_CraneRig_Arm",
    L"/Engine/EditorMeshes/Camera/SM_CraneRig_Base.SM_CraneRig_Base",
    L"/Engine/EditorMeshes/Camera/SM_CraneRig_Body.SM_CraneRig_Body",
    L"/Engine/EditorMeshes/Camera/SM_CraneRig_Mount.SM_CraneRig_Mount",
    L"/Engine/EditorMeshes/Camera/SM_RailRig_Mount.SM_RailRig_Mount",
    L"/Engine/EditorMeshes/Camera/SM_RailRig_Track.SM_RailRig_Track",
    L"/Game/Pickups/Ammo/Model/Meshes/SM_ammo_pistol.SM_ammo_pistol",
};

static const std::wstring OHL_NEWS_ITEM_URL =
    L"https://raw.githubusercontent.com/apple1417/OpenHotfixLoader/master/news_icon.png";

/**
 * @brief Struct holding all data about a given mod file.
 * @note Only one of path or url should have a value, the other should always be empty.
 */
struct mod_file_data {
    std::optional<std::filesystem::path> path = std::nullopt;
    std::optional<std::wstring> url = std::nullopt;
    std::vector<hotfix> hotfixes{};
    std::vector<hotfix> type_11_hotfixes{};
    std::unordered_set<std::wstring> type_11_maps{};
    std::vector<news_item> news_items{};

    /**
     * @brief Checks if this mod file data is empty.
     *
     * @return True if empty, false otherwise.
     */
    bool is_empty(void) {
        return this->hotfixes.size() == 0 && this->type_11_hotfixes.size() == 0 &&
               this->type_11_maps.size() == 0 && this->news_items.size() == 0;
    }
};

static void load_mod_file(const std::filesystem::path&, std::vector<mod_file_data>&);
static void load_mod_url(const std::wstring&, std::vector<mod_file_data>&);

// This default works, relative to the cwd, we'll try replace it later.
static std::filesystem::path mod_dir = "ohl-mods";

static std::mutex reloading_mutex;

static std::vector<mod_file_data> mod_files{};
static std::deque<hotfix> injected_hotfixes{};
static std::deque<news_item> injected_news_items{};

/**
 * @brief Loads mod from a stream.
 *
 * @param stream The stream to read from.
 * @param default_data A mod data struct with metadata pre-initalized. Will be copied as needed.
 * @param file_list List of mod file data to append to.
 */
static void load_mod_stream(std::istream& stream,
                            const mod_file_data& default_data,
                            std::vector<mod_file_data>& file_list) {
    auto mod_data = default_data;
    for (std::string narrow_mod_line; std::getline(stream, narrow_mod_line);) {
        std::wstring mod_line = ohl::util::widen(narrow_mod_line);

        auto whitespace_end_pos = mod_line.find_first_not_of(WHITESPACE);
        if (whitespace_end_pos == std::wstring::npos) {
            continue;
        }

        auto lower_mod_line = mod_line;
        std::transform(lower_mod_line.begin(), lower_mod_line.end(), lower_mod_line.begin(),
                       std::towlower);

        /**
         * @brief Checks if the current line starts with the specified command.
         * @note Should compare against lowercase strings.
         *
         * @param str The command string to check.
         * @return True if the line starts with the command, false otherwise.
         */
        auto is_command = [&](auto str) {
            return lower_mod_line.compare(whitespace_end_pos, str.size(), str) == 0;
        };

        if (is_command(HOTFIX_COMMAND)) {
            auto hotfix_type_end_pos = mod_line.find_first_of(',', whitespace_end_pos);
            if (hotfix_type_end_pos == std::wstring::npos) {
                continue;
            }

            auto hotfix_type =
                mod_line.substr(whitespace_end_pos, hotfix_type_end_pos - whitespace_end_pos);
            auto hotfix = mod_line.substr(hotfix_type_end_pos + 1);

            if (is_command(TYPE_11_PREFIX)) {
                auto map_start_pos = whitespace_end_pos + TYPE_11_PREFIX.size();
                auto map_end_pos = mod_line.find_first_of(')', map_start_pos);
                if (map_end_pos != std::string::npos) {
                    auto map = mod_line.substr(map_start_pos, map_end_pos - map_start_pos);
                    mod_data.type_11_maps.insert(map);
                    mod_data.type_11_hotfixes.push_back({hotfix_type, hotfix});
                    continue;
                }
            }

            mod_data.hotfixes.push_back({hotfix_type, hotfix});
        } else if (is_command(NEWS_COMMAND)) {
            /**
             * @brief Extracts a csv-escaped field from the current line.
             *
             * @param start_pos The position of the first character of the field.
             * @return A pair of the extracted string, and the position of the next field (or npos).
             */
            auto extract_csv_escaped = [&](auto start_pos) -> std::pair<std::wstring, size_t> {
                // If the field is not escaped, simple comma search + return
                if (mod_line[start_pos] != '"') {
                    auto comma_pos = mod_line.find_first_of(',', start_pos);
                    return {mod_line.substr(start_pos, comma_pos - start_pos),
                            comma_pos == std::string::npos ? std::string::npos : comma_pos + 1};
                }

                std::wstringstream stream{};
                auto quoted_start_pos = start_pos + 1;
                auto quoted_end_pos = std::string::npos;
                while (true) {
                    quoted_end_pos = mod_line.find_first_of('"', quoted_start_pos);
                    stream << mod_line.substr(quoted_start_pos, quoted_end_pos - quoted_start_pos);

                    // If we reached the end of the line
                    if (quoted_end_pos >= (mod_line.size() - 1)) {
                        break;
                    }

                    // If this is not an escaped quote
                    if (mod_line[quoted_end_pos + 1] != '"') {
                        break;
                    }

                    stream << '"';
                    // This might move past the end of the string, but will be caught on next loop
                    quoted_start_pos = quoted_end_pos + 2;
                }

                auto comma_pos = mod_line.find_first_of(',', quoted_end_pos);
                return {stream.str(),
                        comma_pos == std::string::npos ? std::string::npos : comma_pos + 1};
            };

            auto header_start_pos = mod_line.find_first_of(',', whitespace_end_pos) + 1;
            auto [header, header_end_pos] = extract_csv_escaped(header_start_pos);

            if (header.size() == 0) {
                continue;
            }

            if (header_end_pos == std::string::npos) {
                mod_data.news_items.push_back({header, L"", L""});
                continue;
            }

            auto [url, url_end_pos] = extract_csv_escaped(header_end_pos);
            mod_data.news_items.push_back(
                {header, url,
                 url_end_pos == std::string::npos ? L"" : mod_line.substr(url_end_pos)});
        } else if (is_command(EXEC_COMMAND)) {
            auto exec_end_pos = whitespace_end_pos + EXEC_COMMAND.size();
            if (WHITESPACE.find(mod_line[exec_end_pos]) == std::string::npos) {
                continue;
            }

            auto path_start = mod_line.find_first_not_of(WHITESPACE, exec_end_pos + 1);
            if (path_start == std::string::npos) {
                continue;
            }

            auto path_end = mod_line.find_last_not_of(WHITESPACE);
            if (path_end == std::string::npos) {
                path_end = mod_line.size() - 1;
            }

            if (mod_line[path_start] == '"' && mod_line[path_end] == '"') {
                path_start++;
                path_end--;
            }

            auto path = (mod_dir / mod_line.substr(path_start, (path_end + 1) - path_start))
                            .lexically_normal();

            // Push our current data, and create a new one for use after loading the file.
            // Required to keep proper ordering, the executed file should act as if it was included
            //  in the middle of the file we're currently reading.
            if (!mod_data.is_empty()) {
                file_list.push_back(mod_data);
                mod_data = default_data;
            }

            load_mod_file(path, file_list);
        } else if (is_command(URL_COMMAND)) {
            auto url = mod_line.substr(whitespace_end_pos + URL_COMMAND.size());

            // Create new data if needed, as above
            if (!mod_data.is_empty()) {
                file_list.push_back(mod_data);
                mod_data = default_data;
            }

            load_mod_url(url, file_list);
        }
    }

    if (!mod_data.is_empty()) {
        file_list.push_back(mod_data);
    }
}

/**
 * @brief Loads all hotfixes in a mod file.
 *
 * @param path Path to the file.
 * @param file_list List of mod file data to append to.
 */
static void load_mod_file(const std::filesystem::path& path,
                          std::vector<mod_file_data>& file_list) {
    // If we've already loaded this file, quit
    if (std::find_if(file_list.begin(), file_list.end(), [&](auto item) {
            return item.path.has_value() && item.path.value() == path;
        }) != file_list.end()) {
        return;
    }

    std::ifstream stream{path};
    if (!stream.is_open()) {
        return;
    }

    mod_file_data mod_data{};
    mod_data.path = path;

    load_mod_stream(stream, mod_data, file_list);
}

/**
 * @brief Loads all hotfixes from a given url.
 *
 * @param url The url to load.
 * @param file_list List of mod file data to append to.
 */
static void load_mod_url(const std::wstring& url, std::vector<mod_file_data>& file_list) {
    // If we've already loaded this url, quit
    if (std::find_if(file_list.begin(), file_list.end(), [&](auto item) {
            return item.url.has_value() && item.url.value() == url;
        }) != file_list.end()) {
        return;
    }

    auto narrow_url = ohl::util::narrow(url);

    // An empty string tells libcurl to accept whatever encodings it can
    auto resp = cpr::Get(cpr::Url{narrow_url}, cpr::AcceptEncoding{{""}});

    if (resp.status_code == 0) {
        std::cout << "[OHL] Error downloading '" << narrow_url << "': " << resp.error.message
                  << "\n";
        return;
    } else if (resp.status_code >= 400) {
        std::cout << "[OHL] Error downloading '" << narrow_url << "': " << resp.status_code << "\n";
        return;
    }

    std::stringstream stream{resp.text};

    mod_file_data mod_data{};
    mod_data.url = url;

    load_mod_stream(stream, mod_data, file_list);
}

/**
 * @brief Creates the news item for OHL, based on the injected hotfixes.
 * @note Should be run after replacing the injected hotfixes list.
 *
 * @return The OHL news item.
 */
static news_item get_ohl_news_item(void) {
    auto size = injected_hotfixes.size();

    // If we're in BL3, colour the name.
    // WL doesn't support font tags :(
    std::wstring ohl_name;
    if (exe_path.stem() == "Borderlands3") {
        ohl_name =
            L"<font color='#0080E0'>OHL</font><font size='14' color='#C0C0C0'> " VERSION_STRING
            "</font>";
    } else {
        ohl_name = L"OHL " VERSION_STRING;
    }

    std::wstring header;
    if (size > 1) {
        header = ohl_name + L": " + std::to_wstring(size) + L" hotfixes loaded";
    } else if (size == 1) {
        header = ohl_name + L": 1 hotfix loaded";
    } else {
        header = ohl_name + L": No hotfixes loaded";
    }

    std::wstring body;
    if (size == 0) {
        body = L"No hotfixes loaded";
    } else {
        std::wstringstream stream{};
        stream << L"Loaded files:\n";

        std::unordered_set<std::filesystem::path> seen_files{};
        std::unordered_set<std::wstring> seen_urls{};
        for (const auto& file_data : mod_files) {
            if (file_data.path.has_value()) {
                auto path = *file_data.path;
                if (seen_files.count(path) > 0) {
                    continue;
                }
                seen_files.insert(path);

                if (path.parent_path() == mod_dir) {
                    stream << path.filename().wstring() << L"\n";
                } else {
                    stream << path.wstring() << L"\n";
                }
            } else {
                auto url = *file_data.url;
                if (seen_urls.count(url) > 0) {
                    continue;
                }
                seen_urls.insert(url);
                stream << url << L"\n";
            }
        }

        body = stream.str();
    }

    return {header, OHL_NEWS_ITEM_URL, body};
}

/**
 * @brief Implementation of `reload`, which reloads the hotfix list.
 * @note Intended to be run in a thread.
 */
static void reload_impl(void) {
    std::lock_guard<std::mutex> lock(reloading_mutex);

    SetThreadDescription(GetCurrentThread(), L"OpenHotfixLoader Loader");

    // If the mod folder doesn't exist, create it, and then just quit early since we know we won't
    //  load anything
    if (!std::filesystem::exists(mod_dir)) {
        std::filesystem::create_directories(mod_dir);
        return;
    }

    mod_files.clear();
    for (const auto& file : ohl::util::get_sorted_files_in_dir(mod_dir)) {
        load_mod_file(file, mod_files);
    }

    injected_hotfixes.clear();
    injected_news_items.clear();
    std::deque<hotfix> type_11_hotfixes;
    std::unordered_set<std::wstring> type_11_maps;
    for (const auto& data : mod_files) {
        injected_hotfixes.insert(injected_hotfixes.end(), data.hotfixes.begin(),
                                 data.hotfixes.end());
        injected_news_items.insert(injected_news_items.end(), data.news_items.begin(),
                                   data.news_items.end());
        type_11_hotfixes.insert(type_11_hotfixes.end(), data.type_11_hotfixes.begin(),
                                data.type_11_hotfixes.end());
        type_11_maps.insert(data.type_11_maps.begin(), data.type_11_maps.end());
    }

    // Add type 11s to the front of the list, and their delays after them but before the rest
    for (const auto& map : type_11_maps) {
        static const auto map_start_pos = TYPE_11_DELAY_VALUE.find(L"{map}");
        static const auto map_length = 5;
        static const auto mesh_start_pos = TYPE_11_DELAY_VALUE.find(L"{mesh}");
        static const auto mesh_length = 6;

        for (const auto& mesh : TYPE_11_DELAY_MESHES) {
            // Make sure to replace mesh first, since it appears later in the string
            auto hotfix = std::wstring(TYPE_11_DELAY_VALUE)
                              .replace(mesh_start_pos, mesh_length, mesh)
                              .replace(map_start_pos, map_length, map);
            type_11_hotfixes.push_back({TYPE_11_DELAY_TYPE, hotfix});
        }
    }
    for (auto it = type_11_hotfixes.rbegin(); it != type_11_hotfixes.rend(); it++) {
        injected_hotfixes.push_front(*it);
    }

    injected_news_items.push_front(get_ohl_news_item());

    std::wcout << L"[OHL] " << injected_news_items[0].body << "\n";
}

void init(void) {
    if (std::filesystem::exists(dll_path)) {
        mod_dir = dll_path.remove_filename() / mod_dir;
    }
}

void reload(void) {
    std::thread thread(reload_impl);
    thread.detach();
}

std::deque<hotfix> get_hotfixes(void) {
    std::lock_guard<std::mutex> lock(reloading_mutex);

    return injected_hotfixes;
}

std::deque<news_item> get_news_items(void) {
    std::lock_guard<std::mutex> lock(reloading_mutex);

    return injected_news_items;
}

}  // namespace ohl::loader
