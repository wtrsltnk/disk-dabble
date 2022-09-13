cmake_minimum_required(VERSION 3.12)

project(disk-dabble VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_compile_definitions(UNICODE _UNICODE _UNICODE_)
add_subdirectory(thirdparty/tiny-process-library)

add_subdirectory(thirdparty/sqlite3)

add_executable(disk-dabble
    include/app.hpp
    include/opendocument.h
    include/openfolderwidget.h
    include/openimagewidget.h
    include/opentextwidget.h
    include/serviceprovider.h
    include/settingsservice.h
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/opendocument.cpp
    src/openfolderwidget.cpp
    src/openimagewidget.cpp
    src/opentextwidget.cpp
    src/program.cpp
    src/serviceprovider.cpp
    src/settingsservice.cpp
    thirdparty/ImGuiColorTextEdit/TextEditor.cpp
    thirdparty/ImGuiColorTextEdit/TextEditor.h
    thirdparty/stb/stb_image.cpp
    thirdparty/stb/stb_image.h
    thirdparty/Davide-Pizzolato/EXIF.CPP
    thirdparty/Davide-Pizzolato/EXIF.H
)

target_compile_features(disk-dabble
    PRIVATE
        cxx_nullptr
        cxx_std_17
)

target_include_directories(disk-dabble
    PRIVATE
        "include"
        "thirdparty/ImGuiColorTextEdit"
        "thirdparty/stb"
        "thirdparty/sqlite3/include"
        "thirdparty/Davide-Pizzolato"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(disk-dabble
    PRIVATE
        glfw
        imgui
        sqlite3
        tiny-process-library
)

target_compile_definitions(disk-dabble
    PRIVATE UNICODE
    PRIVATE _UNICODE
    PRIVATE _UNICODE_
)
