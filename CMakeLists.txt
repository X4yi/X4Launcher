cmake_minimum_required(VERSION 3.20)
project(X4Launcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Include cmake modules
include(cmake/CompilerFlags.cmake)

# Find Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Concurrent)

# Subdirectories (order matters for dependencies)
add_subdirectory(src/core)
add_subdirectory(src/network)
add_subdirectory(src/auth)
add_subdirectory(src/modrinth)
add_subdirectory(src/java)
add_subdirectory(src/cache)
add_subdirectory(src/instance)
add_subdirectory(src/launch)
add_subdirectory(src/ui)
add_subdirectory(tools)

# Main executable
add_executable(X4Launcher src/main.cpp resources/resources.qrc)

target_link_libraries(X4Launcher PRIVATE
    x4_core
    x4_network
    x4_modrinth
    x4_java
    x4_cache
    x4_instance
    x4_launch
    x4_ui
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# --- Installation ---
# Set binary name based on platform
if(WIN32)
    set(EXE_NAME "X4Launcher.exe")
else()
    set(EXE_NAME "X4Launcher")
endif()

install(TARGETS X4Launcher 
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install assets
install(FILES LICENSE.txt DESTINATION .)
install(FILES README.md DESTINATION .)

if(NOT WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.desktop")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.desktop" DESTINATION share/applications)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.png")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.png" DESTINATION share/icons/hicolor/256x256/apps)
    endif()
endif()

# --- CPack Configuration ---
set(CPACK_PACKAGE_NAME "X4Launcher")
set(CPACK_PACKAGE_VENDOR "X4")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "X4Launcher - A modern Minecraft Launcher")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
