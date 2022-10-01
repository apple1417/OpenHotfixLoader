#include <pch.h>

namespace ohl::args {

bool debug;
bool dump_hotfixes;

void parse(void) {
    auto args = std::wstring(GetCommandLine());

    debug = args.find(L"--ohl-debug") != std::string::npos;
    dump_hotfixes = args.find(L"--dump-hotfixes") != std::string::npos;
}

}  // namespace ohl::args
