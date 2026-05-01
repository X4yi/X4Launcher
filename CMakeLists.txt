cmake_minimum_required(VERSION 3.20)
project(X4Launcher VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

# Include cmake modules
include(cmake/CompilerFlags.cmake)

set(X4_QT_ROOT "" CACHE PATH "Qt installation prefix containing lib/cmake/Qt6")
set(X4_QT_HOST_PATH "" CACHE PATH "Host Qt installation prefix used by Qt tools when cross-compiling")

foreach(_env_qt_root IN ITEMS X4_QT_ROOT Qt6_ROOT QT_ROOT_DIR)
  if(NOT X4_QT_ROOT AND DEFINED ENV{${_env_qt_root}} AND NOT "$ENV{${_env_qt_root}}" STREQUAL "")
    set(X4_QT_ROOT "$ENV{${_env_qt_root}}" CACHE PATH "Qt installation prefix containing lib/cmake/Qt6" FORCE)
  endif()
endforeach()

if(NOT X4_QT_HOST_PATH AND DEFINED ENV{X4_QT_HOST_PATH} AND NOT "$ENV{X4_QT_HOST_PATH}" STREQUAL "")
  set(X4_QT_HOST_PATH "$ENV{X4_QT_HOST_PATH}" CACHE PATH "Host Qt installation prefix used by Qt tools when cross-compiling" FORCE)
endif()

set(_x4_qt_prefix_hints)
foreach(_qt_prefix IN LISTS X4_QT_ROOT CMAKE_PREFIX_PATH)
  if(_qt_prefix AND EXISTS "${_qt_prefix}")
    list(APPEND _x4_qt_prefix_hints
      "${_qt_prefix}"
      "${_qt_prefix}/lib/cmake"
      "${_qt_prefix}/lib/cmake/Qt6"
    )
  endif()
endforeach()
list(PREPEND CMAKE_PREFIX_PATH ${_x4_qt_prefix_hints})

if(CMAKE_CROSSCOMPILING AND X4_QT_HOST_PATH)
  set(QT_HOST_PATH "${X4_QT_HOST_PATH}" CACHE PATH "Host Qt path used by cross builds" FORCE)
endif()

# Find Qt6
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)

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

# Windows application icon
set(X4_EXTRA_SOURCES)
if(WIN32)
    set(WIN_RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/x4launcher.rc")
    if(EXISTS "${WIN_RC_FILE}")
        list(APPEND X4_EXTRA_SOURCES "${WIN_RC_FILE}")
    endif()
endif()

# Main executable
add_executable(X4Launcher WIN32
    src/main.cpp
    resources/resources.qrc
    ${X4_EXTRA_SOURCES}
)

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
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install assets
install(FILES LICENSE.txt DESTINATION .)
install(FILES README.md DESTINATION .)

if(NOT WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.desktop")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.desktop" DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.png")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/deploy/x4launcher.png" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
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
