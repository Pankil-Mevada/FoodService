add_executable(PaymentService
    src/main.cpp
    src/Payment.cpp
    src/PaymentController.cpp
    src/PaymentService.cpp
    src/PaymentRepository.cpp
    src/RazorpayClient.cpp
    src/client/NotificationClient.cpp
    src/client/OrderClient.cpp
)

target_include_directories(PaymentService PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(PaymentService PRIVATE
    Common
    CURL::libcurl
    OpenSSL::Crypto
)
find_package(CURL CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
