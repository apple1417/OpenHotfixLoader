#include <pch.h>

#include "dllmain.h"
#include "loader.h"
#include "util.h"
#include "version.h"

using mod_file_identifier = std::wstring;

namespace ohl::loader {

class mod_file;
class mod_manager;

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

static const std::wstring OHL_NEWS_ITEM_IMAGE_URL =
    L"https://raw.githubusercontent.com/apple1417/OpenHotfixLoader/master/news_icon.png";

static const std::wstring OHL_NEWS_ITEM_ARTICLE_URL =
    L"https://github.com/apple1417/OpenHotfixLoader/releases";

// This default works relative to the cwd, we'll try replace it later.
static std::filesystem::path mod_dir = "ohl-mods";

static std::mutex known_mod_files_mutex;
static std::unordered_map<mod_file_identifier, std::shared_ptr<mod_file>> known_mod_files{};

/**
 * @brief Class holding all the data that can be extracted from a region of a mod file.
 */
class mod_data {
   public:
    std::deque<hotfix> hotfixes{};
    std::vector<hotfix> type_11_hotfixes{};
    std::unordered_set<std::wstring> type_11_maps{};
    std::deque<news_item> news_items{};

    /**
     * @brief Checks if this object holds no mod data.
     *
     * @return True if empty, false otherwise.
     */
    bool is_empty(void) const {
        return this->hotfixes.size() == 0 && this->type_11_hotfixes.size() == 0 &&
               this->type_11_maps.size() == 0 && this->news_items.size() == 0;
    }

    /**
     * @brief Appends the mod data from this object to the end of that of another.
     *
     * @param other The other mod data object to append to.
     */
    void append_to(mod_data& other) const {
        other.hotfixes.insert(other.hotfixes.end(), this->hotfixes.begin(), this->hotfixes.end());
        other.type_11_hotfixes.insert(other.type_11_hotfixes.end(), this->type_11_hotfixes.begin(),
                                      this->type_11_hotfixes.end());
        other.type_11_maps.insert(this->type_11_maps.begin(), this->type_11_maps.end());
        other.news_items.insert(other.news_items.end(), this->news_items.begin(),
                                this->news_items.end());
    }
};

/**
 * @brief Class representing mod data coming from another file.
 */
class remote_mod_data {
   public:
    mod_file_identifier identifier;
};

/**
 * @brief Base class holding a section of a mod file, and some metadata about it.
 */
class mod_file {
   protected:
    void load_from_stream(std::istream& stream, bool allow_exec);

    /**
     * @brief Adds a remote to this file's sections, as well as to the global list of known files.
     * @note If the file was not previously known, starts loading it.
     * @note Edits the global list atomically.
     *
     * @param file The file to register.
     */
    void register_remote_file(const std::shared_ptr<mod_file> file) {
        auto identifier = file->get_identifier();

        remote_mod_data remote;
        remote.identifier = identifier;
        this->sections.push_back(remote);

        bool is_known = true;

        {
            std::lock_guard<std::mutex> lock(known_mod_files_mutex);

            if (known_mod_files.find(identifier) == known_mod_files.end()) {
                known_mod_files[identifier] = file;
                is_known = false;
            }
        }

        if (!is_known) {
            file->load();
        }
    }

   public:
    std::vector<std::variant<mod_data, remote_mod_data>> sections;
    /**
     * @brief Appends all the mod data from this file to the end of a mod data object.
     * @note Recursively looks up remote files.
     * @note Calls join, will block until the data is ready.
     *
     * @param data The mod data object to append to.
     * @param seen_files A list of files which have already been seen. Any files which have yet to
     *                    be seen will be appended in the order encountered.
     */
    void append_to(mod_data& data, std::vector<mod_file_identifier>& seen_files) {
        this->join();

        for (const auto& section : this->sections) {
            if (std::holds_alternative<mod_data>(section)) {
                std::get<mod_data>(section).append_to(data);
            } else {
                auto file = known_mod_files.at(std::get<remote_mod_data>(section).identifier);

                auto identifier = file->get_identifier();
                if (std::find(seen_files.begin(), seen_files.end(), identifier) !=
                    seen_files.end()) {
                    LOGD << "[OHL] Already seen " << identifier;
                    continue;
                } else {
                    LOGD << "[OHL] Appending mod data for " << identifier;
                }
                seen_files.push_back(identifier);

                file->append_to(data, seen_files);
            }
        }
    }

    /**
     * @brief Gets a unique identifier for this mod file.
     *
     * @return The mod's identifier.
     */
    virtual mod_file_identifier get_identifier(void) const = 0;

    /**
     * @brief Gets the display name of this mod file.
     *
     * @return The mod's display name.
     */
    virtual std::wstring get_display_name(void) const = 0;

    /**
     * @brief Attempts to load the contents of this mod file.
     * @note May be async, must call join to ensure this has finished.
     */
    virtual void load(void) = 0;

    /**
     * @brief Joins any threads started by loading this mod file.
     */
    virtual void join(void) {}
};

/**
 * @brief Class for mod file data based on a local file.
 */
class mod_file_local : public mod_file {
   public:
    std::filesystem::path path;

    virtual mod_file_identifier get_identifier(void) const { return this->path.wstring(); }

    virtual std::wstring get_display_name(void) const {
        if (this->path.parent_path() == mod_dir) {
            return this->path.filename().wstring();
        } else {
            return this->path.wstring();
        }
    }

    virtual void load(void) {
        LOGD << "[OHL] Loading " << path;

        std::ifstream stream{path};
        if (!stream.is_open()) {
            LOGE << "[OHL]: Error opening file '" << path;
            return;
        }

        this->load_from_stream(stream, true);
    }
};

/**
 * @brief Class for mod file data based on a url.
 */
class mod_file_url : public mod_file {
   private:
    std::future<void> download;

   public:
    std::wstring url;

    virtual mod_file_identifier get_identifier(void) const { return this->url; }

    virtual std::wstring get_display_name(void) const {
        auto last_slash_pos = this->url.find_last_of('/');
        if (last_slash_pos == std::string::npos) {
            return this->url;
        } else {
            return this->url.substr(last_slash_pos + 1) + L" (url)";
        }
    }

    virtual void load(void) {
        LOGD << "[OHL] Loading " << this->url;

        this->download = cpr::GetCallback(
            [&](const cpr::Response& resp) {
                LOGD << "[OHL] Finished downloading " << this->url;

                if (resp.status_code == 0) {
                    LOGE << "[OHL] Error downloading '" << this->url << "': " << resp.error.message;
                    return;
                } else if (resp.status_code >= 400) {
                    LOGE << "[OHL] Error downloading '" << this->url << "': " << resp.status_code;
                    return;
                }

                std::stringstream stream{resp.text};

                this->load_from_stream(stream, false);
            },
            cpr::Url{ohl::util::narrow(this->url)},
            // An empty string tells libcurl to accept whatever encodings it can
            cpr::AcceptEncoding{{""}});
    };

    virtual void join(void) {
        if (!this->download.valid()) {
            throw std::runtime_error("Tried to join a url download before starting it!");
        }
        this->download.get();
    }
};

/**
 * @brief Class for the `ohl-mods` mod "file".
 * @note Inheriting from the mod file base class for the merging logic.
 */
class mods_folder : public mod_file {
   public:
    virtual mod_file_identifier get_identifier(void) const {
        throw std::runtime_error("Mods folder should not be treated as a mod file!");
    }

    virtual std::wstring get_display_name(void) const {
        throw std::runtime_error("Mods folder should not be treated as a mod file!");
    }

    virtual void load(void) {
        LOGI << "[OHL] Loading mods folder";

        for (const auto& path : ohl::util::get_sorted_files_in_dir(mod_dir)) {
            auto file = std::make_shared<mod_file_local>();
            file->path = path;
            this->register_remote_file(file);
        }
    }
};

/**
 * @brief Loads this mod file from a stream.
 *
 * @param stream The stream to read from.
 * @param allow_exec True if to allow running exec commands.
 */
void mod_file::load_from_stream(std::istream& stream, bool allow_exec) {
    mod_data data{};

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
                    data.type_11_maps.insert(map);
                    data.type_11_hotfixes.push_back({hotfix_type, hotfix});
                    continue;
                }
            }

            data.hotfixes.push_back({hotfix_type, hotfix});
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

            if (header_end_pos == std::string::npos) {
                data.news_items.push_back({header, L"", L"", L""});
                continue;
            }

            auto [image_url, image_url_end_pos] = extract_csv_escaped(header_end_pos);
            if (image_url_end_pos == std::string::npos) {
                data.news_items.push_back({header, image_url, L"", L""});
                continue;
            }

            auto [article_url, article_url_end_pos] = extract_csv_escaped(image_url_end_pos);
            data.news_items.push_back({header, image_url, article_url,
                                       article_url_end_pos == std::string::npos
                                           ? L""
                                           : mod_line.substr(article_url_end_pos)});
        } else if (allow_exec && is_command(EXEC_COMMAND)) {
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
            if (!data.is_empty()) {
                sections.push_back(data);
                mod_data new_data{};
                data = new_data;
            }

            auto new_file = std::make_shared<mod_file_local>();
            new_file->path = path;
            this->register_remote_file(new_file);
        } else if (is_command(URL_COMMAND)) {
            auto url = mod_line.substr(whitespace_end_pos + URL_COMMAND.size());

            // Create new data if needed, as above
            if (!data.is_empty()) {
                sections.push_back(data);
                mod_data new_data{};
                data = new_data;
            }

            auto new_file = std::make_shared<mod_file_url>();
            new_file->url = url;
            this->register_remote_file(new_file);
        }
    }

    if (!data.is_empty()) {
        this->sections.push_back(data);
    }
}

/**
 * @brief Creates the news item for OHL, based on the given mod data.
 *
 * @param data The mod data to base the news off of.
 * @param file_order The injected mod files, in the order they were loaded.
 * @return The OHL news item.
 */
static news_item get_ohl_news_item(const mod_data& data,
                                   std::vector<std::shared_ptr<mod_file>> file_order) {
    auto size = data.hotfixes.size();

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

        for (const auto& file : file_order) {
            stream << file->get_display_name() << L",\n";
        }

        body = stream.str();
    }

    return {header, OHL_NEWS_ITEM_IMAGE_URL, OHL_NEWS_ITEM_ARTICLE_URL, body};
}

static std::mutex reloading_mutex;
static mod_data loaded_mod_data;

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

    // No need to lock here since we haven't started loading
    known_mod_files.clear();

    mods_folder folder_data{};
    folder_data.load();

    LOGD << "[OHL] Combining mod data";
    mod_data combined_mod_data{};
    std::vector<mod_file_identifier> seen_files;
    folder_data.append_to(combined_mod_data, seen_files);

    LOGD << "[OHL] Processing type 11s";

    // Add type 11s to the front of the list, and their delays after them but before the rest
    for (const auto& map : combined_mod_data.type_11_maps) {
        static const auto map_start_pos = TYPE_11_DELAY_VALUE.find(L"{map}");
        static const auto map_length = 5;
        static const auto mesh_start_pos = TYPE_11_DELAY_VALUE.find(L"{mesh}");
        static const auto mesh_length = 6;

        for (const auto& mesh : TYPE_11_DELAY_MESHES) {
            // Make sure to replace mesh first, since it appears later in the string
            auto hotfix = std::wstring(TYPE_11_DELAY_VALUE)
                              .replace(mesh_start_pos, mesh_length, mesh)
                              .replace(map_start_pos, map_length, map);
            combined_mod_data.type_11_hotfixes.push_back({TYPE_11_DELAY_TYPE, hotfix});
        }
    }
    for (auto it = combined_mod_data.type_11_hotfixes.rbegin();
         it != combined_mod_data.type_11_hotfixes.rend(); it++) {
        combined_mod_data.hotfixes.push_front(*it);
    }

    LOGD << "[OHL] Adding OHL news item";

    std::vector<std::shared_ptr<mod_file>> file_order;
    for (const auto& identifier : seen_files) {
        auto file = known_mod_files.at(identifier);
        if (file->sections.size() == 0) {
            continue;
        }

        // Special case ignoring a file holding a single remote url reference - i.e. the url
        //  shortcut files we'll be seeing in 99% of cases
        if (file->sections.size() == 1 &&
            std::holds_alternative<remote_mod_data>(file->sections[0])) {
            auto remote_file =
                known_mod_files.at(std::get<remote_mod_data>(file->sections[0]).identifier);
            if (dynamic_cast<mod_file_url*>(remote_file.get()) != nullptr) {
                continue;
            }
        }

        file_order.push_back(file);
    }

    combined_mod_data.news_items.push_front(get_ohl_news_item(combined_mod_data, file_order));

    LOGD << "[OHL] Replacing globals";

    loaded_mod_data = combined_mod_data;

    LOGI << "[OHL] Loading finished, loaded files:";
    for (const auto& file : file_order) {
        LOGI << "[OHL] " << file->get_display_name();
    }
}

void init(void) {
    if (std::filesystem::exists(dll_path)) {
        mod_dir = std::filesystem::path(dll_path).remove_filename() / mod_dir;
    }
}

void reload(void) {
    std::thread thread(reload_impl);
    thread.detach();
}

std::deque<hotfix> get_hotfixes(void) {
    std::lock_guard<std::mutex> lock(reloading_mutex);

    return loaded_mod_data.hotfixes;
}

std::deque<news_item> get_news_items(void) {
    std::lock_guard<std::mutex> lock(reloading_mutex);

    return loaded_mod_data.news_items;
}

}  // namespace ohl::loader
