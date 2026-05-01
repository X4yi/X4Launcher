# Modrinth API integration module

add_library(x4_modrinth STATIC
        ModrinthAPI.cpp
)

target_link_libraries(x4_modrinth
        PUBLIC
        Qt6::Core
        Qt6::Network
        x4_core
)
