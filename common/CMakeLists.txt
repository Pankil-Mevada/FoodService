add_library(Common
    src/Database.cpp
)

target_include_directories(Common
    PUBLIC
        include
)

target_link_libraries(Common
    PUBLIC
        SQLite::SQLite3
)
