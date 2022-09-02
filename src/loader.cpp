#include <pch.h>

#include "dllmain.h"
#include "loader.h"
#include "util.h"
#include "version.h"

namespace ohl::loader {

static const std::wstring HOTFIX_COMMAND = L"Spark";

static const std::wstring TYPE_11_PREFIX = L"SparkEarlyLevelPatchEntry,(1,11,0,";
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

struct mod_file_data {
    std::filesystem::path path;
    std::vector<hotfix> hotfixes{};
    std::vector<hotfix> type_11_hotfixes{};
    std::unordered_set<std::wstring> type_11_maps{};
    std::vector<news_item> news_items{};
};

// This default works, relative to the cwd, we'll try replace it later.
static std::filesystem::path mod_dir = "ohl-mods";

static std::mutex reloading_mutex;

static std::vector<mod_file_data> mod_files{};
static std::deque<hotfix> injected_hotfixes{};
static std::deque<news_item> injected_news_items{};

/**
 * @brief Loads all hotfixes in a mod file.
 *
 * @param path Path to the file.
 * @param file_list List of mod file data to append to.
 */
static void load_mod_file(const std::filesystem::path& path,
                          std::vector<mod_file_data>& file_list) {
    std::ifstream mod_file{path};
    if (!mod_file.is_open()) {
        std::wcout << L"[OHL] Failed to open file '" << path << L"'!\n";
        return;
    }

    mod_file_data mod_data{};
    mod_data.path = path;

    for (std::string narrow_mod_line; std::getline(mod_file, narrow_mod_line);) {
        std::wstring mod_line = ohl::util::widen(narrow_mod_line);

        auto whitespace_end_pos = mod_line.find_first_not_of(L" \f\n\r\t\b");
        if (whitespace_end_pos == std::wstring::npos) {
            continue;
        }

        auto is_command = [&](auto str) {
            return mod_line.compare(whitespace_end_pos, str.size(), str) == 0;
        };

        if (is_command(HOTFIX_COMMAND)) {
            auto hotfix_type_end_pos = mod_line.find_first_of(',');
            if (hotfix_type_end_pos == std::wstring::npos) {
                continue;
            }

            auto hotfix_type =
                mod_line.substr(whitespace_end_pos, hotfix_type_end_pos - whitespace_end_pos);
            auto hotfix = mod_line.substr(hotfix_type_end_pos + 1);

            if (mod_line.rfind(TYPE_11_PREFIX) == 0) {
                auto map_end_pos = mod_line.find_first_of(')');
                if (map_end_pos != std::string::npos) {
                    auto map =
                        mod_line.substr(TYPE_11_PREFIX.size(), map_end_pos - TYPE_11_PREFIX.size());
                    mod_data.type_11_maps.insert(map);
                    mod_data.type_11_hotfixes.push_back({hotfix_type, hotfix});
                    continue;
                }
            }

            mod_data.hotfixes.push_back({hotfix_type, hotfix});
        }
    }

    file_list.push_back(mod_data);
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
        for (const auto& file_data : mod_files) {
            if (seen_files.count(file_data.path) > 0) {
                continue;
            }
            seen_files.insert(file_data.path);

            if (file_data.path.parent_path() != mod_dir) {
                stream << file_data.path.wstring() << L"\n";
            } else {
                stream << file_data.path.filename().wstring() << L"\n";
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
