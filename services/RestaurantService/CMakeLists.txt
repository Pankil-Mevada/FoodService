add_executable(RestaurantService
    src/main.cpp
    src/RestaurantController.cpp
    src/Restaurant.cpp
    src/RestaurantService.cpp
    src/RestaurantRepository.cpp
    src/PartnerController.cpp
    src/PartnerRepository.cpp
)


target_include_directories(RestaurantService PRIVATE
    include
      ${PROJECT_SOURCE_DIR}/common/include
    ${PROJECT_SOURCE_DIR}/third_party/Crow/include
)

target_link_libraries(RestaurantService PRIVATE
	Common
)
