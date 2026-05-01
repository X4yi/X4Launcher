add_library(x4_launch STATIC
        VersionManifest.cpp
        VersionParser.cpp
        ArgumentResolver.cpp
        MetaAPI.cpp
        LaunchEngine.cpp
)

target_include_directories(x4_launch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_launch PUBLIC x4_core x4_network x4_cache x4_java x4_instance x4_auth Qt6::Core Qt6::Network)
