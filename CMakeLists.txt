cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(OpenHotfixLoader VERSION 1.4)
set(PROJECT_VERSION_PREFIX "v")
set(CMAKE_DEBUG_POSTFIX "-Debug")

# CPR turns this on, so we need to force it back off
option(BUILD_SHARED_LIBS "" OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/user-includes.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/user-includes.cmake")
endif()

add_subdirectory(cpr)
add_subdirectory(minhook)
add_subdirectory(plog)

if (DEFINED CMAKE_MINGW32)
    set(CMAKE_CXX_EXTENSIONS False)
    find_library(shlwapi libshlwapi REQUIRED)
    set(CMAKE_EXE_COMPILER_FLAGS  "${CMAKE_EXE_COMPILER_FLAGS} -std=c++17 -pthread")
    set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive -pthread -lshlwapi")
else()
    find_library(shlwapi Shlwapi.lib REQUIRED)
endif()



configure_file(versioninfo.rc.in versioninfo.rc)
configure_file(src/version.h.in inc/version.h)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")

add_library(OpenHotfixLoader SHARED ${sources} "${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc")
target_include_directories(OpenHotfixLoader PRIVATE "${PROJECT_BINARY_DIR}/inc" "src")

if (DEFINED CMAKE_MINGW32)
   target_link_libraries(OpenHotfixLoader PUBLIC minhook libshlwapi.a cpr::cpr plog)
else()
   target_link_libraries(OpenHotfixLoader PUBLIC minhook shlwapi cpr::cpr plog)
endif()


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

# Postbuild
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postbuild.bat")
    add_custom_command(
        TARGET OpenHotfixLoader
        POST_BUILD
        COMMAND postbuild.bat "\"$<SHELL_PATH:$<TARGET_FILE:OpenHotfixLoader>>\" \"$<IF:$<CONFIG:DEBUG>,DEBUG,RELEASE>\""
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
