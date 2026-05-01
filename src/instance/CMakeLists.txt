add_library(x4_instance STATIC
        InstanceConfig.cpp
        InstanceImporter.cpp
        InstanceManager.cpp
        ModLoader.cpp
)

target_include_directories(x4_instance PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_instance PUBLIC x4_core x4_network Qt6::Core Qt6::Network)
