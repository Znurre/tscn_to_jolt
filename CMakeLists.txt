cmake_minimum_required(VERSION 3.24)

include(FetchContent)
FetchContent_Declare(_project_options URL https://github.com/aminya/project_options/archive/refs/tags/v0.41.0.zip)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)

run_vcpkg(
    VCPKG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg
)

project(tscn_to_jolt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glm CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

find_path(CPP_PEGLIB_INCLUDE_DIRS "peglib.h")
find_path(HAVOC_INCLUDE_DIRS "havoc/havoc.hpp")
find_path(GD_PARSER_INCLUDE_DIRS "gd_parser/gd_parser.hpp")

add_executable(${PROJECT_NAME}
    main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CPP_PEGLIB_INCLUDE_DIRS}
    ${HAVOC_INCLUDE_DIRS}
    ${GD_PARSER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm-header-only
    Jolt::Jolt
    spdlog::spdlog
    CLI11::CLI11
)
