cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

project(OpenHotfixLoader VERSION 1.5)
set(PROJECT_VERSION_PREFIX "v")

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_DEBUG_POSTFIX "-Debug")

# CPR turns this on, so we need to force it back off
option(BUILD_SHARED_LIBS "" OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user-includes.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/user-includes.cmake")
endif()

# Libraries
add_subdirectory(cpr)
add_subdirectory(minhook)
add_subdirectory(plog)

set(LIBSHLWAPI "Shlwapi.lib")
if(MINGW)
    # Mingw has different names for shlwapi
    set(LIBSHLWAPI "libshlwapi")
endif()

find_library(shlwapi ${LIBSHLWAPI} REQUIRED)

# Sources
configure_file(versioninfo.rc.in versioninfo.rc)
configure_file(src/version.h.in inc/version.h)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

add_library(OpenHotfixLoader SHARED ${sources} "${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc")
target_include_directories(OpenHotfixLoader PRIVATE "${PROJECT_BINARY_DIR}/inc" "src")

target_link_libraries(OpenHotfixLoader PUBLIC minhook shlwapi cpr::cpr plog)

# CMake by default defines NDEBUG in release, we also want the opposite
target_compile_definitions(OpenHotfixLoader PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
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

if (MINGW)
    # Want to link statically into a single dll
    target_link_options(OpenHotfixLoader PUBLIC "-static")
endif()

# Postbuild
set(POSTBUILD_SCRIPT "postbuild")
if(CMAKE_HOST_WIN32)
	set(POSTBUILD_SCRIPT "${POSTBUILD_SCRIPT}.bat")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${POSTBUILD_SCRIPT}")
    add_custom_command(
        TARGET OpenHotfixLoader
        POST_BUILD
        COMMAND ${POSTBUILD_SCRIPT} "\"$<SHELL_PATH:$<TARGET_FILE:OpenHotfixLoader>>\" \"$<IF:$<CONFIG:DEBUG>,DEBUG,RELEASE>\""
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
