cmake_minimum_required(VERSION 3.20)

project(DoomOpenGL
	VERSION 1.0.0
	LANGUAGES C CXX
)

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

FetchContent_Declare(
	json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.11.3
	GIT_SHALLOW TRUE
)

# Native audio playback; no Python or runtime asset pack.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR unused-cmake-directory
)
FetchContent_MakeAvailable(glfw json miniaudio)
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
