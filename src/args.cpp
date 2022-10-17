#include <pch.h>

#include <doctest/doctest.h>

namespace ohl::args {
TEST_SUITE_BEGIN("args");

typedef struct {
    bool debug;
    bool dump_hotfixes;
    std::filesystem::path exe_path;
    std::filesystem::path dll_path;
} args_t;

static args_t args = {false, false, "", ""};

/**
 * @brief Implementation of `parse`, allowing passing in a custom string.
 *
 * @param cmd The command line args.
 */
static void parse(std::wstring cmd) {
    args.debug = cmd.find(L"--ohl-debug") != std::string::npos;
    args.dump_hotfixes = cmd.find(L"--dump-hotfixes") != std::string::npos;
}

TEST_CASE("args::parse_str") {
    args.debug = false;
    args.dump_hotfixes = false;

    SUBCASE("debug") {
        parse(L"example.exe");
        REQUIRE(args.debug == false);

        parse(L"example.exe --debug");
        REQUIRE(args.debug == false);

        parse(L"example.exe --dump-hotfixes");
        REQUIRE(args.debug == false);

        parse(L"example.exe --ohl-debug");
        REQUIRE(args.debug == true);
    }

    SUBCASE("dump") {
        parse(L"example.exe");
        REQUIRE(args.dump_hotfixes == false);

        parse(L"example.exe --dump-hotfixes");
        REQUIRE(args.dump_hotfixes == true);
    }

    SUBCASE("debug + dump") {
        parse(L"example.exe");
        REQUIRE(args.debug == false);
        REQUIRE(args.dump_hotfixes == false);

        parse(L"example.exe --ohl-debug --dump-hotfixes");
        REQUIRE(args.debug == true);
        REQUIRE(args.dump_hotfixes == true);
    }
}

void init(HMODULE this_module) {
    parse(GetCommandLine());

    wchar_t buf[FILENAME_MAX];
    if (GetModuleFileName(NULL, buf, sizeof(buf))) {
        args.exe_path = std::filesystem::path(std::wstring(buf));
    }
    if (GetModuleFileName(this_module, buf, sizeof(buf))) {
        args.dll_path = std::filesystem::path(std::wstring(buf));
    }
}

bool debug(void) {
    return args.debug;
}

bool dump_hotfixes(void) {
    return args.dump_hotfixes;
}

std::filesystem::path exe_path(void) {
    return args.exe_path;
}

std::filesystem::path dll_path(void) {
    return args.dll_path;
}

TEST_SUITE_END();
}  // namespace ohl::args
