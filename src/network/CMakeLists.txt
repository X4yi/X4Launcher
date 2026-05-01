add_library(x4_network STATIC
        HttpClient.cpp
        DownloadTask.cpp
        DownloadManager.cpp
)

target_include_directories(x4_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_network PUBLIC x4_core Qt6::Core Qt6::Network)
