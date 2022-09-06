#include <pch.h>

namespace ohl::args {

bool debug;
bool dump_hotfixes;

void parse(void) {
    auto args = std::wstring(GetCommandLine());

#ifdef DEBUG
    debug = true;
#else
    debug = args.find(L"--debug") != std::string::npos;
#endif

    dump_hotfixes = args.find(L"--dump-hotfixes") != std::string::npos;
}

}  // namespace ohl::args
