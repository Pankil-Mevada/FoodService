add_executable(RestaurantService
    src/main.cpp
    src/RestaurantController.cpp
    src/Restaurant.cpp
    src/RestaurantService.cpp
    src/RestaurantRepository.cpp
)


target_include_directories(RestaurantService PRIVATE
    include
      ${PROJECT_SOURCE_DIR}/common/include
    ${PROJECT_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(RestaurantService PRIVATE
	Common
)
