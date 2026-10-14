cmake_minimum_required(VERSION 3.20)

project(DoomOpenGL
	VERSION 1.0.0
	LANGUAGES CXX
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
	glad
	GIT_REPOSITORY https://github.com/Dav1dde/glad.git
	GIT_TAG v0.1.36
	GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glad)

find_package(OpenGL REQUIRED)

add_executable(doom_opengl
	src/main.cpp
)

target_link_libraries(doom_opengl
	PRIVATE
		glfw
		glad
		OpenGL::GL
)

target_compile_features(doom_opengl PRIVATE cxx_std_17)

if(MSVC)
	target_compile_options(doom_opengl PRIVATE /W4 /permissive-)
else()
	target_compile_options(doom_opengl PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(doom_opengl PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT doom_opengl)
