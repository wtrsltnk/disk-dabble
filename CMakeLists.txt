cmake_minimum_required(VERSION 3.12)

project(disk-dabble VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(disk-dabble
    include/app.hpp
    include/opendocument.h
    include/openfolderwidget.h
    include/openimagewidget.h
    include/opentextwidget.h
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/opendocument.cpp
    src/openfolderwidget.cpp
    src/openimagewidget.cpp
    src/opentextwidget.cpp
    src/program.cpp
    thirdparty/ImGuiColorTextEdit/TextEditor.cpp
    thirdparty/ImGuiColorTextEdit/TextEditor.h
    thirdparty/stb/stb_image.cpp
    thirdparty/stb/stb_image.h
)

target_compile_features(disk-dabble
    PRIVATE
        cxx_nullptr
        cxx_std_17
)

target_include_directories(disk-dabble
    PRIVATE
        "include"
        "thirdparty"
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
