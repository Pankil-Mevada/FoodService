cmake_minimum_required(VERSION 3.16)

project(ApiGateway)

add_executable(ApiGateway
    src/main.cpp
    src/client/OrderClient.cpp
    src/client/HttpClient.cpp
    src/client/RestaurantClient.cpp
)

target_include_directories(ApiGateway PRIVATE
    include
    ${PROJECT_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(ApiGateway PRIVATE
    Common
    CURL::libcurl
)

find_package(CURL CONFIG REQUIRED)