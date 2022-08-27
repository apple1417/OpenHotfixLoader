cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(OpenHotfixLoader VERSION 0.1.0)
set(PROJECT_VERSION_PREFIX "b")
set(CMAKE_DEBUG_POSTFIX "-Debug")

add_subdirectory(minhook)

configure_file(versioninfo.rc.in versioninfo.rc)
configure_file(src/version.h.in inc/version.h)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

# Can't use pybind11_add_module cause Visual Studio won't let you debug it
add_library(OpenHotfixLoader SHARED ${sources} "${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc")
target_include_directories(OpenHotfixLoader PRIVATE "${PROJECT_BINARY_DIR}/inc" "src")

target_link_libraries(OpenHotfixLoader PUBLIC minhook)

# CMake by default defines NDEBUG in release, we also want the opposite
target_compile_definitions(OpenHotfixLoader PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>")
target_compile_definitions(OpenHotfixLoader PRIVATE "UNICODE" "_UNICODE")

# The precompiled header must be defined AFTER the compile defines
target_precompile_headers(OpenHotfixLoader PUBLIC "src/pch.c" "src/pch.cpp")

if(MSVC)
    # Enable Edit and Continue.
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    target_compile_options(OpenHotfixLoader PUBLIC "$<$<CONFIG:DEBUG>:/ZI>")
    target_link_options(OpenHotfixLoader PUBLIC "/INCREMENTAL")
endif()

# Postbuild
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postbuild.bat")
    add_custom_command(
        TARGET OpenHotfixLoader
        POST_BUILD
        COMMAND postbuild.bat "\"$<SHELL_PATH:$<TARGET_FILE:OpenHotfixLoader>>\" \"$<IF:$<CONFIG:DEBUG>,DEBUG,RELEASE>\""
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
