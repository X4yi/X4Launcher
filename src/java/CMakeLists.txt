add_library(x4_java STATIC
        JavaDetector.cpp
        JavaProvisioner.cpp
        JavaManager.cpp
)

target_include_directories(x4_java PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_java PUBLIC
        x4_core
        x4_network
        Qt6::Core
        Qt6::Network
        Qt6::Concurrent
)
