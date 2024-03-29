cmake_minimum_required(VERSION 3.12)

project(disk-dabble VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_compile_definitions(UNICODE _UNICODE _UNICODE_)

add_subdirectory(thirdparty/sqlite3)

add_executable(disk-dabble
    include/app.hpp
    include/opendocument.h
    include/openfindwidget.h
    include/openfolderwidget.h
    include/openimagewidget.h
    include/opentextwidget.h
    include/serviceprovider.h
    include/settingsservice.h
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/opendocument.cpp
    src/openfindwidget.cpp
    src/openfolderwidget.cpp
    src/openimagewidget.cpp
    src/opentextwidget.cpp
    src/pagesdocument.cpp
    src/pagesdocument.h
    src/program.cpp
    src/serviceprovider.cpp
    src/settingsservice.cpp
    thirdparty/Davide-Pizzolato/EXIF.CPP
    thirdparty/Davide-Pizzolato/EXIF.H
    thirdparty/stb/stb_image.cpp
    thirdparty/stb/stb_image.h
    thirdparty/tiny-process-library/process.cpp

)

if(WIN32)
  target_sources(disk-dabble PRIVATE thirdparty/tiny-process-library/process_win.cpp)
  #If compiled using MSYS2, use sh to run commands
  if(MSYS)
    target_compile_definitions(tiny-process-library PUBLIC MSYS_PROCESS_USE_SH)
  endif()
else()
  target_sources(disk-dabble PRIVATE thirdparty/tiny-process-library/process_unix.cpp)
endif()

target_compile_features(disk-dabble
    PRIVATE
        cxx_nullptr
        cxx_std_17
)

target_include_directories(disk-dabble
    PRIVATE
        "include"
        "thirdparty/stb"
        "thirdparty/sqlite3/include"
        "thirdparty/Davide-Pizzolato"
        "thirdparty/tiny-process-library"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(disk-dabble
    PRIVATE
        glfw
        imgui
        sqlite3
        fmt
        tinyxml2
        miniz
)

target_compile_definitions(disk-dabble
    PRIVATE UNICODE
    PRIVATE _UNICODE
    PRIVATE _UNICODE_
)
