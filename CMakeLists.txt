cmake_minimum_required(VERSION 3.19)
project(poplar_trie)

option(POPLAR_FORCE_RELEASE "Force a release build" OFF)
option(POPLAR_DISABLE_SSE4_2 "Disable SSE 4.2" OFF)
option(POPLAR_VERBOSE "Emit diagnostic info" ON)
option(POPLAR_EXTRA_STATS "Compute extra statistics" OFF)
option(POPLAR_ENABLE_TESTING "Enable testing" OFF)
option(POPLAR_ENABLE_BENCHMARK "Enable benchmark" OFF)
option(POPLAR_ENABLE_SAMPLE "Enable sample" OFF)

if (POPLAR_FORCE_RELEASE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

# C++17 compiler check
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.0)
        message(FATAL_ERROR "Clang 4.0 (or greater) is required for C++17")
    endif()
    set(POPLAR_CLANG 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.0)
        message(FATAL_ERROR "gcc 7.0 (or greater) is required for C++17")
    endif()
    set(POPLAR_GCC 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(POPLAR_INTEL 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(POPLAR_MSVC 1)
endif()

set(BUILTIN_POPCNT 0)

if (POPLAR_DISABLE_SSE4_2)
  message(STATUS "sse4.2 disabled")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Use /proc/cpuinfo to get the information
  file(STRINGS "/proc/cpuinfo" _cpuinfo)
  if(_cpuinfo MATCHES "(sse4_2)|(sse4a)")
    set(BUILTIN_POPCNT 1)
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  execute_process(COMMAND sysctl -n machdep.cpu.features OUTPUT_VARIABLE _cpuinfo OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(_cpuinfo MATCHES "SSE4.2")
    set(BUILTIN_POPCNT 1)
  endif()
endif()

if(BUILTIN_POPCNT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
endif()

if (POPLAR_GCC OR POPLAR_CLANG)
    set(GCC_WARNINGS "-Wall -Werror=return-type")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -pthread ${GCC_WARNINGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -DDEBUG")
endif()

if (POPLAR_VERBOSE)
    message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    message(STATUS "CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
    message(STATUS "CXX_FLAGS_DEBUG are ${CMAKE_CXX_FLAGS_DEBUG}")
    message(STATUS "CXX_FLAGS_RELEASE are ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

add_library(poplar INTERFACE)
target_include_directories(poplar INTERFACE
    $<BUILD_INTERFACE:"${CMAKE_CURRENT_SOURCE_DIR}/include">)
add_library(poplar::poplar ALIAS poplar)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/poplar_config.hpp.in      # in
  ${CMAKE_BINARY_DIR}/include/poplar/poplar_config.hpp  # out
)
install(FILES
    ${CMAKE_BINARY_DIR}/include/poplar/poplar_config.hpp 
    DESTINATION include/poplar)
file(GLOB POPLAR_HEADERS "include/poplar/*.hpp")
install(FILES include/poplar.hpp DESTINATION include)
install(FILES ${POPLAR_HEADERS} DESTINATION include/poplar)
install(TARGETS poplar
    EXPORT poplarConfig
    PUBLIC_HEADER DESTINATION include)
install(EXPORT poplarConfig
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/poplar"
    NAMESPACE poplar:: )

if (POPLAR_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()

if (POPLAR_ENABLE_BENCHMARK)
    add_subdirectory(bench)
endif()
if (POPLAR_ENABLE_SAMPLE)
    add_subdirectory(sample)
endif()



