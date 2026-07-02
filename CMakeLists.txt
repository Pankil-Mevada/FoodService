cmake_minimum_required(VERSION 3.16)

project(FoodService VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
add_subdirectory(common)
add_subdirectory(services/UserService)
