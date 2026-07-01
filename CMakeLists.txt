cmake_minimum_required(VERSION 3.16)
project(untitled134)

set(CMAKE_CXX_STANDARD 20)

find_package(SQLite3 REQUIRED)

add_executable(untitled134
        main.cpp
        Database.cpp
        Database.hpp
        Models.hpp
        App.cpp
        App.hpp
)

target_link_libraries(untitled134 PRIVATE SQLite::SQLite3)
