cmake_minimum_required(VERSION 3.20)

project(sqlite3 VERSION "5.2.2")

add_library(sqlite3)

target_compile_features(sqlite3
    PRIVATE
        cxx_std_17
)

# Add src
target_sources(sqlite3
    PRIVATE
        include/sqlite3.h
        include/sqlitelib.h
        src/sqlite3.c
)

target_include_directories(sqlite3
    PUBLIC
        "include"
)
