cmake_minimum_required(VERSION 3.16)

project(FoodService VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
#find_package(argon2 CONFIG REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

add_subdirectory(common)
add_subdirectory(services/UserService)
add_subdirectory(services/RestaurantService)
add_subdirectory(services/OrderService)
add_subdirectory(services/PaymentService)
add_subdirectory(services/NotificationService)
