add_library(Common
    src/Database.cpp
    src/UserValidator.cpp
    src/JwtManager.cpp
)

target_include_directories(Common
    PUBLIC
        include
)

target_link_libraries(Common
    PUBLIC
        SQLite::SQLite3
	#argon2::argon2
	unofficial::argon2::libargon2
        jwt-cpp::jwt-cpp
)
