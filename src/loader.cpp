#include <pch.h>

#include "dllmain.h"
#include "loader.h"
#include "util.h"
#include "version.h"

// Type used to store data about loaded hotfix files
// While a map would seem cleaner, it loses load order, we need to preserve it
using hotfix_file_data_list =
    std::vector<std::pair<std::filesystem::path, std::filesystem::file_time_type>>;

namespace ohl::loader {

static const std::wstring HOTFIX_PREFIX = L"Spark";

static const std::wstring TYPE_11_PREFIX = L"SparkEarlyLevelPatchEntry,(1,11,0,";
static const std::wstring TYPE_11_DELAY =
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

// Again this default works, relative to the cwd, we'll try replace it later.
static std::filesystem::path mod_dir = "ohl-mods";

static std::deque<hotfix> injected_hotfixes{};
static std::deque<news_item> injected_news_items{};
static hotfix_file_data_list hotfix_files{};

/**
 * @brief Loads all hotfixes in a mod file.
 *
 * @param path Path to the file.
 * @param hotfixes A list of hotfixes.
 * @param type_11_hotfixes A list of type 11 hotfixes.
 * @param type_11_maps A set of maps on which type 11 hotfixes exist.
 */
static void load_mod_file(const std::filesystem::path& path,
                          std::deque<hotfix>& hotfixes,
                          std::deque<hotfix>& type_11_hotfixes,
                          std::unordered_set<std::wstring>& type_11_maps) {
    std::ifstream mod_file{path};
    if (!mod_file.is_open()) {
        std::wcout << L"[OHL] Failed to open file '" << path << L"'!\n";
        return;
    }

    for (std::string narrow_mod_line; std::getline(mod_file, narrow_mod_line);) {
        std::wstring mod_line = ohl::util::widen(narrow_mod_line);

        auto whitespace_end_pos = mod_line.find_first_not_of(L" \f\n\r\t\b");
        if (whitespace_end_pos == std::wstring::npos) {
            continue;
        }
        if (mod_line.compare(whitespace_end_pos, HOTFIX_PREFIX.size(), HOTFIX_PREFIX) != 0) {
            continue;
        }

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
                type_11_maps.insert(map);
                type_11_hotfixes.push_back({hotfix_type, hotfix});
                continue;
            }
        }

        hotfixes.push_back({hotfix_type, hotfix});
    }
}

/**
 * @brief Loads all hotfixes in a directory.
 *
 * @param path Path to the directory.
 * @param loaded_files A list of files which had hotfixes loaded, in the order they were loaded.
 * @param hotfixes A list of extracted hotfixes.
 * @param type_11_hotfixes A list of extracted type 11 hotfixes.
 * @param type_11_maps A set of maps on which type 11 hotfixes exist.
 * @return True if any hotfixes were loaded, false otherwise
 */
static void load_mod_dir(const std::filesystem::path& path,
                         std::vector<std::filesystem::path>& loaded_files,
                         std::deque<hotfix>& hotfixes,
                         std::deque<hotfix>& type_11_hotfixes,
                         std::unordered_set<std::wstring>& type_11_maps) {
    for (const auto& file : ohl::util::get_sorted_files_in_dir(path)) {
        if (std::find(loaded_files.begin(), loaded_files.end(), file) != loaded_files.end()) {
            continue;
        }
        load_mod_file(file, hotfixes, type_11_hotfixes, type_11_maps);
        loaded_files.push_back(file);
    }
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
        for (const auto& [file, _] : hotfix_files) {
            if (file.parent_path() != mod_dir) {
                stream << file.wstring() << L"\n";
            } else {
                stream << file.filename().wstring() << L"\n";
            }
        }
        body = stream.str();
    }

    return {header, OHL_NEWS_ITEM_URL, body};
}

void init(void) {
    if (std::filesystem::exists(dll_path)) {
        mod_dir = dll_path.remove_filename() / mod_dir;
    }
}

void reload(void) {
    // If the mod folder doesn't exist, create it, and then just quit early since we know we won't
    //  load anything
    if (!std::filesystem::exists(mod_dir)) {
        std::filesystem::create_directories(mod_dir);
        return;
    }

    // Cache a set of last modify times, and only actually reload when a file gets updated.
    if (hotfix_files.size() > 0) {
        // Last write time doesn't update on new/deleted files in a dir, so check by grabbing a list
        std::unordered_set<std::filesystem::path> current_file_list{};
        for (const auto& dir_entry : std::filesystem::directory_iterator{mod_dir}) {
            if (dir_entry.is_directory()) {
                continue;
            }
            current_file_list.insert(dir_entry.path());
        }

        bool any_modified = false;
        for (const auto& [file, last_time] : hotfix_files) {
            // If we fail to erase, the file got deleted
            if (current_file_list.erase(file) == 0) {
                any_modified = true;
                break;
            }
            if (std::filesystem::last_write_time(file) > last_time) {
                any_modified = true;
                break;
            }
        }
        if (!any_modified && current_file_list.size() == 0) {
            return;
        }
    }

    std::vector<std::filesystem::path> loaded_files{};
    std::deque<hotfix> new_hotfixes{};
    std::deque<hotfix> type_11_hotfixes{};
    std::unordered_set<std::wstring> type_11_maps{};

    load_mod_dir(mod_dir, loaded_files, new_hotfixes, type_11_hotfixes, type_11_maps);

    // Add type 11s to the front of the list, and their delays after them but before the rest
    for (const auto& map : type_11_maps) {
        static const auto map_start_pos = TYPE_11_DELAY.find(L"{map}");
        static const auto map_length = 5;
        static const auto mesh_start_pos = TYPE_11_DELAY.find(L"{mesh}");
        static const auto mesh_length = 6;

        for (const auto& mesh : TYPE_11_DELAY_MESHES) {
            // Make sure to replace mesh first, since it appears later in the string
            auto hotfix = std::wstring(TYPE_11_DELAY)
                              .replace(mesh_start_pos, mesh_length, mesh)
                              .replace(map_start_pos, map_length, map);
            type_11_hotfixes.push_back({L"SparkEarlyLevelPatchEntry", hotfix});
        }
    }
    for (auto it = type_11_hotfixes.rbegin(); it != type_11_hotfixes.rend(); it++) {
        new_hotfixes.push_front(*it);
    }

    hotfix_file_data_list new_hotfix_files{};
    for (const auto& file : loaded_files) {
        new_hotfix_files.push_back({file, std::filesystem::last_write_time(file)});
    }

    injected_hotfixes = new_hotfixes;
    hotfix_files = new_hotfix_files;
    injected_news_items = {get_ohl_news_item()};

    std::wcout << L"[OHL] " << injected_news_items[0].body << "\n";
}

std::deque<hotfix> get_hotfixes(void) {
    return injected_hotfixes;
}

std::deque<news_item> get_news_items(void) {
    return injected_news_items;
}

}  // namespace ohl::loader
