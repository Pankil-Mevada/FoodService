cmake_minimum_required(VERSION 3.16)

project(FoodService VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

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
add_subdirectory(services/ApiGateway)

if(BUILD_TESTING)
    add_executable(delivery_quote_test tests/delivery_quote_test.cpp)
    target_include_directories(delivery_quote_test PRIVATE
        ${CMAKE_SOURCE_DIR}/services/ApiGateway/include)
    add_test(NAME delivery_quote COMMAND delivery_quote_test)

    add_executable(payment_order_status_test tests/payment_order_status_test.cpp)
    target_include_directories(payment_order_status_test PRIVATE
        ${CMAKE_SOURCE_DIR}/services/OrderService/include)
    add_test(NAME payment_order_status COMMAND payment_order_status_test)

    add_executable(http_result_test tests/http_result_test.cpp)
    target_include_directories(http_result_test PRIVATE
        ${CMAKE_SOURCE_DIR}/services/ApiGateway/include)
    add_test(NAME http_result COMMAND http_result_test)

    add_executable(correlation_id_test tests/correlation_id_test.cpp)
    target_include_directories(correlation_id_test PRIVATE
        ${CMAKE_SOURCE_DIR}/services/ApiGateway/include)
    add_test(NAME correlation_id COMMAND correlation_id_test)

    add_executable(account_security_test tests/account_security_test.cpp)
    target_link_libraries(account_security_test PRIVATE Common)
    add_test(NAME account_security COMMAND account_security_test)
endif()
