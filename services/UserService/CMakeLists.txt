find_package(SQLite3 REQUIRED)
add_executable(UserService
    src/main.cpp
    src/UserController.cpp
    src/User.cpp
    src/UserService.cpp
    src/UserRepository.cpp
    src/Database.cpp
)


target_include_directories(UserService PRIVATE
    include
    ${PROJECT_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(UserService PRIVATE
    SQLite::SQLite3
)
