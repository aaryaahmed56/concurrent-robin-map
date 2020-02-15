cmake_minimum_required(VERSION 3.0.0)
project(crh VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(crh INTERFACE)

target_include_directories(crh INTERFACE
                           "$<BUILD_INTERFACE: ${CMAKE_CURRENT_SOURCE_DIR}/crh>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

list(APPEND headers "${CMAKE_CURRENT_SOURCE_DIR}/crh/util/backoff.hpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crh/util/constraints.hpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crh/util/forwarding_constructors.hpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crh/util/utils.hpp"
                    "${CMAKE_CURRENT_SOURCE_DIR}/crh/detail/concurrent_robin_map.hpp")
target_sources(crh INTERFACE "$<BUILD_INTERFACE:${headers}>")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
