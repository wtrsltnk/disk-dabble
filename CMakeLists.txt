cmake_minimum_required(VERSION 3.12)

project(diskpry VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(diskpry
    include/app.hpp
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/program.cpp
)

target_compile_features(diskpry
    PRIVATE
        cxx_nullptr
)

target_include_directories(diskpry
    PRIVATE
        "include"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(diskpry
    PRIVATE
        glfw
        imgui
)


