add_library(x4_cache STATIC
    LibraryCache.cpp
    AssetCache.cpp
    CacheManager.cpp
)

target_include_directories(x4_cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(x4_cache PUBLIC x4_core x4_network Qt6::Core)
