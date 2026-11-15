cmake_minimum_required(VERSION 3.20)

project(DoomOpenGL
	VERSION 1.0.0
	LANGUAGES C CXX
)

# Enable languages at the top level before GLFW/miniaudio create their targets.
# On macOS, a late enable_language(OBJCXX) can leave CMake generation rules
# undefined for sources already added in dependency subdirectories.
if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.4
	GIT_SHALLOW TRUE
)

# Avoid fetching repository history for the header-only JSON dependency.
# Official release archive is about 111 KB, with its published SHA-256.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip remote dependency updates")
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Show dependency download progress")
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    TIMEOUT 120
    TLS_VERIFY ON
)
message(STATUS "Preparing GLFW 3.4...")
FetchContent_MakeAvailable(glfw)
message(STATUS "Preparing JSON 3.11.3...")
FetchContent_MakeAvailable(json)

# Native audio only needs one header, not the entire miniaudio repository.
# Pin the exact commit previously used by the 0.11.21 tag.
message(STATUS "Preparing miniaudio 0.11.21...")
if(FETCHCONTENT_SOURCE_DIR_MINIAUDIO)
    set(miniaudio_SOURCE_DIR "${FETCHCONTENT_SOURCE_DIR_MINIAUDIO}")
elseif(EXISTS "${FETCHCONTENT_BASE_DIR}/miniaudio-src/miniaudio.h")
    # Reuse the 0.11.21 checkout from previous builds.
    set(miniaudio_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/miniaudio-src")
else()
    set(miniaudio_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/miniaudio-0.11.21")
    set(audio_header "${miniaudio_SOURCE_DIR}/miniaudio.h")
    if(NOT EXISTS "${audio_header}")
        file(MAKE_DIRECTORY "${miniaudio_SOURCE_DIR}")
        file(DOWNLOAD
            https://raw.githubusercontent.com/mackron/miniaudio/4a5b74bef029b3592c54b6048650ee5f972c1a48/miniaudio.h
            "${audio_header}.part"
            TLS_VERIFY ON TIMEOUT 120 INACTIVITY_TIMEOUT 30
            SHOW_PROGRESS STATUS audio_download)
        list(GET audio_download 0 audio_status)
        if(NOT audio_status EQUAL 0)
            file(REMOVE "${audio_header}.part")
            message(FATAL_ERROR "miniaudio download failed: ${audio_download}. Retry CMake or set FETCHCONTENT_SOURCE_DIR_MINIAUDIO.")
        endif()
        file(RENAME "${audio_header}.part" "${audio_header}")
    endif()
endif()
if(NOT EXISTS "${miniaudio_SOURCE_DIR}/miniaudio.h")
    message(FATAL_ERROR "miniaudio.h is missing from ${miniaudio_SOURCE_DIR}")
endif()
message(STATUS "Dependencies ready.")
find_package(Threads REQUIRED)
add_library(doom_audio STATIC src/Audio.cpp src/miniaudio_impl.c)
target_include_directories(doom_audio PRIVATE "${miniaudio_SOURCE_DIR}")
target_link_libraries(doom_audio PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX AND NOT APPLE)
    target_link_libraries(doom_audio PRIVATE m)
endif()

find_package(OpenGL REQUIRED)

# GLFW 3.4 already ships pre-generated GLAD 2 bindings for OpenGL 3.3.
# Compile them directly: no Python interpreter or code-generation step.
add_library(doom_glad STATIC src/glad_loader.c)
target_include_directories(doom_glad PUBLIC "${glfw_SOURCE_DIR}/deps")

# Share the exact same JSON validation between the game and the validator.
add_library(level_core src/Level.cpp)
target_include_directories(level_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(level_core PUBLIC nlohmann_json::nlohmann_json)

add_executable(doom_opengl src/main.cpp)
target_link_libraries(doom_opengl PRIVATE glfw doom_glad OpenGL::GL level_core doom_audio)
if(APPLE)
    set_target_properties(doom_opengl PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
    target_sources(doom_opengl PRIVATE src/NativeWindow.mm)
    set_source_files_properties(src/NativeWindow.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    target_link_libraries(doom_opengl PRIVATE ${APPKIT_FRAMEWORK})
endif()

# Standalone command-line tool; no OpenGL window or Python needed.
add_executable(level_validator tools/validate_levels.cpp)
target_link_libraries(level_validator PRIVATE level_core)

foreach(target_name IN ITEMS level_core doom_opengl level_validator)
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

set_target_properties(doom_opengl PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT doom_opengl)

# Support launching from the build directory as well as the repository root.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/assets/levels")
foreach(level_file IN ITEMS level1.json level2.json level3.json level4.json test_arena.json platforming.json)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/levels/${level_file}"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/levels/${level_file}"
        COPYONLY
    )
endforeach()
enable_testing()
add_executable(platform_tests tools/platform_tests.cpp)
target_link_libraries(platform_tests PRIVATE level_core)
add_test(NAME platform_collision COMMAND platform_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/levels/platforming.json")

add_executable(combat_tests tools/combat_tests.cpp)
target_include_directories(combat_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_test(NAME combat_balance COMMAND combat_tests)
