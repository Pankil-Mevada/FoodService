cmake_minimum_required(VERSION 3.16)

project(OrderService)

add_executable(OrderService
    src/main.cpp
    src/OrderController.cpp
    src/Order.cpp
    src/OrderService.cpp
    src/OrderRepository.cpp
)

target_include_directories(OrderService PRIVATE
    include
    ${PROJECT_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(OrderService PRIVATE
    Common
)
