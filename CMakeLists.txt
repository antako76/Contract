cmake_minimum_required(VERSION 3.16)

project(contract
    VERSION 0.3.0
    DESCRIPTION "Compile-time C++ data contract layer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CONTRACT_ENABLE_SPAN 0)
if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    set(CONTRACT_ENABLE_SPAN 1)
endif()

option(CONTRACT_BUILD_EXAMPLES "Build CONTRACT examples" ON)
option(CONTRACT_BUILD_TESTS "Build CONTRACT tests" ON)
option(CONTRACT_BUILD_BENCHMARKS "Build CONTRACT benchmarks" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CONTRACT_PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_library(contract INTERFACE)
add_library(contract::contract ALIAS contract)

target_include_directories(contract
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(contract INTERFACE cxx_std_20)
target_compile_definitions(contract INTERFACE
    $<$<BOOL:${CONTRACT_ENABLE_SPAN}>:CONTRACT_ENABLE_SPAN=1>
)

if(CONTRACT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CONTRACT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CONTRACT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

install(
    TARGETS contract
    EXPORT contractTargets
)

install(
    DIRECTORY include/contract
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT contractTargets
    FILE contractTargets.cmake
    NAMESPACE contract::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contract
)

configure_package_config_file(
    cmake/contractConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/contractConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contract
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/contractConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/contractConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/contractConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/contract
)

install(
    FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/contract
)
