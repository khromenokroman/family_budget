cmake_minimum_required(VERSION 3.16)
project(family_budget)

set(CMAKE_CXX_STANDARD 20)

find_package(SQLite3 REQUIRED)

add_executable(family_budget
        src/main.cpp
        src/Database.cpp
        src/Database.hpp
        src/Models.hpp
        src/App.cpp
        src/App.hpp
)

target_link_libraries(family_budget PRIVATE SQLite::SQLite3)
