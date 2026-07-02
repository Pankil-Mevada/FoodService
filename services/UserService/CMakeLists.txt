add_executable(UserService
    src/main.cpp
    src/UserController.cpp
    src/User.cpp
    src/UserService.cpp
)


target_include_directories(UserService PRIVATE
    include
    ${PROJECT_SOURCE_DIR}/third_party/Crow/include
)
