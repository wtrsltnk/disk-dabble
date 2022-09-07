cmake_minimum_required(VERSION 3.12)

project(disk-dabble VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(disk-dabble
    include/app.hpp
    include/openfolderwidget.h
    include/openimagewidget.h
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/openfolderwidget.cpp
    src/openimagewidget.cpp
    src/program.cpp
)

target_compile_features(disk-dabble
    PRIVATE
        cxx_nullptr
        cxx_std_17
)

target_include_directories(disk-dabble
    PRIVATE
        "include"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(disk-dabble
    PRIVATE
        glfw
        imgui
)

target_compile_definitions(disk-dabble
    PRIVATE _UNICODE
    PRIVATE UNICODE
    PRIVATE _UNICODE_
)
