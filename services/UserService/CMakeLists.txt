add_executable(UserService
    src/main.cpp
)

target_include_directories(UserService PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/Crow/include
)
