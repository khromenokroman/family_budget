cmake_minimum_required(VERSION 3.16)
project(family_budget)

set(CMAKE_CXX_STANDARD 20)

find_package(SQLite3 REQUIRED)

add_library(budget_core STATIC
        src/database.cpp
        src/database.hpp
        src/models.hpp
)
target_include_directories(budget_core PUBLIC src)
target_link_libraries(budget_core PUBLIC SQLite::SQLite3)

add_executable(family_budget
        src/main.cpp
        src/app.cpp
        src/app.hpp
)
target_link_libraries(family_budget PRIVATE budget_core)

enable_testing()
find_package(GTest REQUIRED)

add_executable(family_budget_tests
        tests/database_test.cpp
)
target_link_libraries(family_budget_tests PRIVATE budget_core GTest::gtest GTest::gtest_main)

add_test(NAME family_budget_tests COMMAND family_budget_tests)
