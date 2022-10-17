#include <pch.h>

#include <doctest/doctest.h>

namespace ohl::args {
TEST_SUITE_BEGIN("args");

bool debug;
bool dump_hotfixes;

/**
 * @brief Implementation of `parse`, allowing passing in a custom string
 *
 * @param args The command line args
 */
static void parse_str(std::wstring args) {
    debug = args.find(L"--ohl-debug") != std::string::npos;
    dump_hotfixes = args.find(L"--dump-hotfixes") != std::string::npos;
}

TEST_CASE("args::parse_str") {
    debug = false;
    dump_hotfixes = false;

    SUBCASE("debug") {
        parse_str(L"example.exe");
        REQUIRE(debug == false);

        parse_str(L"example.exe --debug");
        REQUIRE(debug == false);

        parse_str(L"example.exe --dump-hotfixes");
        REQUIRE(debug == false);

        parse_str(L"example.exe --ohl-debug");
        REQUIRE(debug == true);
    }

    SUBCASE("dump") {
        parse_str(L"example.exe");
        REQUIRE(dump_hotfixes == false);

        parse_str(L"example.exe --dump-hotfixes");
        REQUIRE(dump_hotfixes == true);
    }

    SUBCASE("debug + dump") {
        parse_str(L"example.exe");
        REQUIRE(debug == false);
        REQUIRE(dump_hotfixes == false);

        parse_str(L"example.exe --ohl-debug --dump-hotfixes");
        REQUIRE(debug == true);
        REQUIRE(dump_hotfixes == true);
    }
}

void parse(void) {
    parse_str(GetCommandLine());
}

TEST_SUITE_END();
}  // namespace ohl::args
