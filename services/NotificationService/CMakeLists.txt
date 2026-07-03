add_executable(NotificationService
    src/main.cpp
    src/Notification.cpp
    src/NotificationController.cpp
    src/NotificationService.cpp
    src/NotificationRepository.cpp
)

target_include_directories(NotificationService PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(NotificationService PRIVATE
    Common
)
