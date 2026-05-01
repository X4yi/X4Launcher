# CMake Toolchain file for cross-compiling to Windows using MinGW-w64
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake ..

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(X4_MINGW_TRIPLET "x86_64-w64-mingw32" CACHE STRING "Target triplet for the MinGW-w64 toolchain")
set(X4_MINGW_ROOT "/usr/${X4_MINGW_TRIPLET}" CACHE PATH "Root path of the MinGW-w64 sysroot")
set(X4_MINGW_BIN_DIR "/usr/bin" CACHE PATH "Directory containing the MinGW-w64 compiler binaries")
set(X4_QT_ROOT "${X4_MINGW_ROOT}" CACHE PATH "Qt for Windows target prefix")

if(DEFINED ENV{X4_MINGW_TRIPLET} AND NOT "$ENV{X4_MINGW_TRIPLET}" STREQUAL "")
  set(X4_MINGW_TRIPLET "$ENV{X4_MINGW_TRIPLET}" CACHE STRING "Target triplet for the MinGW-w64 toolchain" FORCE)
endif()
if(NOT X4_MINGW_TRIPLET)
  set(X4_MINGW_TRIPLET "x86_64-w64-mingw32" CACHE STRING "Target triplet for the MinGW-w64 toolchain" FORCE)
endif()

if(DEFINED ENV{X4_MINGW_ROOT} AND NOT "$ENV{X4_MINGW_ROOT}" STREQUAL "")
  set(X4_MINGW_ROOT "$ENV{X4_MINGW_ROOT}" CACHE PATH "Root path of the MinGW-w64 sysroot" FORCE)
endif()
if(NOT X4_MINGW_ROOT)
  set(X4_MINGW_ROOT "/usr/${X4_MINGW_TRIPLET}" CACHE PATH "Root path of the MinGW-w64 sysroot" FORCE)
endif()

if(DEFINED ENV{X4_MINGW_BIN_DIR} AND NOT "$ENV{X4_MINGW_BIN_DIR}" STREQUAL "")
  set(X4_MINGW_BIN_DIR "$ENV{X4_MINGW_BIN_DIR}" CACHE PATH "Directory containing the MinGW-w64 compiler binaries" FORCE)
endif()
if(NOT X4_MINGW_BIN_DIR)
  set(X4_MINGW_BIN_DIR "/usr/bin" CACHE PATH "Directory containing the MinGW-w64 compiler binaries" FORCE)
endif()

if(DEFINED ENV{X4_QT_ROOT} AND NOT "$ENV{X4_QT_ROOT}" STREQUAL "")
  set(X4_QT_ROOT "$ENV{X4_QT_ROOT}" CACHE PATH "Qt for Windows target prefix" FORCE)
endif()
if(NOT X4_QT_ROOT)
  set(X4_QT_ROOT "${X4_MINGW_ROOT}" CACHE PATH "Qt for Windows target prefix" FORCE)
endif()

if(DEFINED ENV{X4_QT_HOST_PATH} AND NOT "$ENV{X4_QT_HOST_PATH}" STREQUAL "")
  set(X4_QT_HOST_PATH "$ENV{X4_QT_HOST_PATH}" CACHE PATH "Host Qt path used by Qt tools" FORCE)
endif()

find_program(CMAKE_C_COMPILER
  NAMES ${X4_MINGW_TRIPLET}-gcc
  HINTS "${X4_MINGW_BIN_DIR}"
  REQUIRED
)
find_program(CMAKE_CXX_COMPILER
  NAMES ${X4_MINGW_TRIPLET}-g++
  HINTS "${X4_MINGW_BIN_DIR}"
  REQUIRED
)
find_program(CMAKE_RC_COMPILER
  NAMES ${X4_MINGW_TRIPLET}-windres
  HINTS "${X4_MINGW_BIN_DIR}"
  REQUIRED
)

set(CMAKE_FIND_ROOT_PATH "${X4_MINGW_ROOT}")
set(CMAKE_SYSROOT "${X4_MINGW_ROOT}")

# Adjust finding behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

list(APPEND CMAKE_PREFIX_PATH
  "${X4_QT_ROOT}"
  "${X4_QT_ROOT}/lib/cmake"
  "${X4_QT_ROOT}/lib/cmake/Qt6"
)

if(X4_QT_HOST_PATH)
  set(QT_HOST_PATH "${X4_QT_HOST_PATH}" CACHE PATH "Host Qt path used by Qt tools" FORCE)
endif()
