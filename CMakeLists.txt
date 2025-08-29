cmake_minimum_required(VERSION 3.30)

# ---------------------------------------------------------------------------------------
# Toolchain Configuration (e.g., vcpkg)
# Must be set before 'project()' for vcpkg or cross-compiling
# ---------------------------------------------------------------------------------------
if (NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE FILEPATH "Vcpkg toolchain file"
  )
endif()

# intel icx by default
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER icx CACHE STRING "C compiler" FORCE)
endif()
if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER icpx CACHE STRING "C++ compiler" FORCE)
endif()

# ---------------------------------------------------------------------------------------
# Project Metadata
# ---------------------------------------------------------------------------------------
project(cc_chan VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DISRUPTOR_VERSION_MAJOR "1")
set(DISRUPTOR_VERSION_MINOR "0")
set(DISRUPTOR_VERSION_PATCH "0")
set(DISRUPTOR_VERSION "${DISRUPTOR_VERSION_MAJOR}.${DISRUPTOR_VERSION_MINOR}.${DISRUPTOR_VERSION_PATCH}")

# ---------------------------------------------------------------------------------------
# Options & Configurable Variables
# ---------------------------------------------------------------------------------------
#set(MY_OPT_FLAGS "" CACHE STRING "Custom optimization flags for Release builds, e.g., -Ofast -march=native")

add_subdirectory(Disruptor)
