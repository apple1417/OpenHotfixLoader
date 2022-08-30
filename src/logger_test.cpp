#include <iostream>
#include <fstream>
#include <sstream>
#include "logger.h"

using namespace ohl::logger;


int main() {
    ohl::logger::logPrint( "testing strings" );
    ohl::logger::logPrint( L"testing wide strings" );
    std::stringstream X;
    X << "Testing a concatnated stringstream";
    ohl::logger::logPrint(X);
    ohl::logger::logPrint(std::stringstream() << "Testing stringstream 1 liner");
    ohl::logger::logPrint(std::stringstream() << "Testing" << " concat string stream");
    // from the code
    std::runtime_error ex("Exception Text");
    std::wstring path = L"/etc";
    ohl::logger::logPrint( "[OHL] Running in debug mode" );
    ohl::logger::logPrint( std::stringstream() << "[OHL] Exception occured during initalization: " << ex.what() );
    ohl::logger::logPrint( std::stringstream() << "[OHL] Exception occured in discovery hook: " << ex.what() );
    ohl::logger::logPrint( std::stringstream() << "[OHL] Exception occured in news hook: " << ex.what() );
    ohl::logger::logPrint( std::stringstream() << "[OHL] Exception occured in image hook: " << ex.what() );
    ohl::logger::logPrint( "[OHL] Hooks injected successfully" );
    ohl::logger::logPrint( std::wstringstream() << L"[OHL] Failed to open file '" << path << L"'!" );
    // ohl::logger::logPrint( std::wstringstream << L"[OHL] " << get_loaded_mods_str() );
    ohl::logger::logPrint( "[OHL] Injected hotfixes" );
    ohl::logger::logPrint( "[OHL] Dumped hotfixes to file" );
    ohl::logger::logPrint( "Didn't find vf tables in time!");
    ohl::logger::logPrint( "[OHL] Injected news" );
    ohl::logger::logPrint( "[OHL] Prevented news icon from being cached" );
    return 0;
}

