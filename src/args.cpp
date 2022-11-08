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
static void parse(std::string cmd) {
    args.debug = cmd.find("--ohl-debug") != std::string::npos;
    args.dump_hotfixes = cmd.find("--dump-hotfixes") != std::string::npos;
}

TEST_CASE("args::parse_str") {
    args.debug = false;
    args.dump_hotfixes = false;

    SUBCASE("debug") {
        parse("example.exe");
        REQUIRE(args.debug == false);

        parse("example.exe --debug");
        REQUIRE(args.debug == false);

        parse("example.exe --dump-hotfixes");
        REQUIRE(args.debug == false);

        parse("example.exe --ohl-debug");
        REQUIRE(args.debug == true);
    }

    SUBCASE("dump") {
        parse("example.exe");
        REQUIRE(args.dump_hotfixes == false);

        parse("example.exe --dump-hotfixes");
        REQUIRE(args.dump_hotfixes == true);
    }

    SUBCASE("debug + dump") {
        parse("example.exe");
        REQUIRE(args.debug == false);
        REQUIRE(args.dump_hotfixes == false);

        parse("example.exe --ohl-debug --dump-hotfixes");
        REQUIRE(args.debug == true);
        REQUIRE(args.dump_hotfixes == true);
    }
}

void init(HMODULE this_module) {
    parse(GetCommandLineA());

    char buf[FILENAME_MAX];
    if (GetModuleFileNameA(NULL, buf, sizeof(buf))) {
        args.exe_path = std::filesystem::path(buf);
    }
    if (GetModuleFileNameA(this_module, buf, sizeof(buf))) {
        args.dll_path = std::filesystem::path(buf);
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
