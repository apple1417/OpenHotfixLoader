#include <pch.h>

namespace ohl::loader {

static std::wstring get_formatted_name(void);

static const std::string MOD_LIST_FILE = "modlist.txt";
static const int HOTFIX_COUNTER_OFFSET = 10000;

static std::wstring news_header = get_formatted_name() + L": No hotfixes injected";
static std::wstring news_body = L"";
static std::vector<std::pair<std::wstring, std::wstring>> hotfixes{};

static std::wstring get_formatted_name(void) {
    // If we're in BL3, colour the name.
    // WL doesn't support font tags :(
    wchar_t buf[256];
    auto res = GetModuleFileName(NULL, buf, sizeof(buf));
    if (res && std::filesystem::path(std::wstring(buf)).stem() == "Borderlands3") {
        return L"<font color='#0080E0'>OHL</font>";
    }

    return L"OHL";
}

void init(void) {
    if (!std::filesystem::exists(MOD_LIST_FILE)) {
        std::ofstream modlist{MOD_LIST_FILE};
        modlist << "# OpenHotfixLoader Mod List\n";
        modlist << "# Add paths to the mod files you want to load here, one per line.\n";
        modlist << "# Lines starting with a '#' are comments and are ignored.\n";
        modlist.close();
        return;
    }

    std::wifstream modlist{MOD_LIST_FILE};
    if (!modlist.is_open()) {
        std::cout << "[OHL] Failed to open file '" << MOD_LIST_FILE << "'!\n";
        return;
    }
    modlist.imbue(
        std::locale(std::locale::empty(), new std::codecvt<char16_t, char, std::mbstate_t>));

    std::wstringstream body_msg{};
    bool has_any_hotfixes = false;
    hotfixes.clear();

    for (std::wstring list_line; std::getline(modlist, list_line);) {
        if (std::all_of(list_line.begin(), list_line.end(), isspace)) {
            continue;
        }

        if (list_line[0] == '#') {
            continue;
        }

        std::wifstream mod_file{list_line};
        if (!mod_file.is_open()) {
            std::wcout << L"[OHL] Failed to open file '" << list_line << L"'!\n";
            continue;
        }
        mod_file.imbue(
            std::locale(std::locale::empty(), new std::codecvt<char16_t, char, std::mbstate_t>));

        bool mod_has_hotfixes = false;
        for (std::wstring mod_line; std::getline(mod_file, mod_line);) {
            if (mod_line.rfind(L"Spark") != 0) {
                continue;
            }

            auto hotfix_type_end_pos = mod_line.find_first_of(',');
            if (hotfix_type_end_pos == std::wstring::npos) {
                continue;
            }

            has_any_hotfixes = true;
            mod_has_hotfixes = true;

            auto hotfix_type = mod_line.substr(0, hotfix_type_end_pos) +
                               std::to_wstring(hotfixes.size() + HOTFIX_COUNTER_OFFSET);
            auto hotfix = mod_line.substr(hotfix_type_end_pos + 1);

            hotfixes.push_back({hotfix_type, hotfix});
        }

        if (mod_has_hotfixes) {
            body_msg << list_line << L"\n";
        }
    }

    if (has_any_hotfixes) {
        news_body = L"Running Mods:\n" + body_msg.str();
        news_header =
            get_formatted_name() + L": " + std::to_wstring(hotfixes.size()) + L" hotfixes loaded";
        std::wcout << L"[OHL] " << news_body;
    }
}

std::vector<std::pair<std::wstring, std::wstring>> get_hotfixes(void) {
    return hotfixes;
}
std::wstring get_news_header(void) {
    return news_header;
}
std::wstring get_news_body(void) {
    return news_body;
}

}  // namespace ohl::loader
