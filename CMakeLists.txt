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

    add_executable(partner_access_policy_test tests/partner_access_policy_test.cpp)
    target_include_directories(partner_access_policy_test PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include)
    add_test(NAME partner_access_policy COMMAND partner_access_policy_test)

    add_executable(restaurant_order_workflow_test tests/restaurant_order_workflow_test.cpp)
    target_include_directories(restaurant_order_workflow_test PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include)
    add_test(NAME restaurant_order_workflow COMMAND restaurant_order_workflow_test)

    add_executable(partner_order_repository_test
        tests/partner_order_repository_test.cpp
        services/OrderService/src/PartnerOrderRepository.cpp)
    target_include_directories(partner_order_repository_test PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/services/OrderService/include)
    target_link_libraries(partner_order_repository_test PRIVATE Common)
    add_test(NAME partner_order_repository COMMAND partner_order_repository_test)

    add_executable(partner_order_controller_test
        tests/partner_order_controller_test.cpp
        services/OrderService/src/PartnerOrderController.cpp
        services/OrderService/src/PartnerOrderRepository.cpp)
    target_include_directories(partner_order_controller_test PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/services/OrderService/include
        ${CMAKE_SOURCE_DIR}/third_party/Crow/include)
    target_link_libraries(partner_order_controller_test PRIVATE Common)
    add_test(NAME partner_order_controller COMMAND partner_order_controller_test)

    add_executable(partner_repository_test
        tests/partner_repository_test.cpp
        services/RestaurantService/src/PartnerRepository.cpp
        services/RestaurantService/src/RestaurantRepository.cpp
        services/RestaurantService/src/Restaurant.cpp)
    target_include_directories(partner_repository_test PRIVATE
        ${CMAKE_SOURCE_DIR}/common/include
        ${CMAKE_SOURCE_DIR}/services/RestaurantService/include)
    target_link_libraries(partner_repository_test PRIVATE Common)
    add_test(NAME partner_repository COMMAND partner_repository_test)
endif()
