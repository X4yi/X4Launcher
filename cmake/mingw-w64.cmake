# CMake Toolchain file for cross-compiling to Windows using MinGW-w64
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Target triple
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Cross compilers
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# Root path for finding dependencies
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Adjust finding behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Qt6 Specific (User might need to adjust this path if Qt6 MinGW is elsewhere)
# If using Arch Linux's mingw-w64-qt6 packages:
set(QT_CMAKE_DIR /usr/${TOOLCHAIN_PREFIX}/lib/cmake)
list(APPEND CMAKE_PREFIX_PATH ${QT_CMAKE_DIR})
# Also try common Qt6 paths
list(APPEND CMAKE_PREFIX_PATH /usr/${TOOLCHAIN_PREFIX}/lib/cmake/Qt6)
list(APPEND CMAKE_PREFIX_PATH /usr/${TOOLCHAIN_PREFIX})
