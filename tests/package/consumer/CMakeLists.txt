cmake_minimum_required(VERSION 3.16)

project(contract_package_consumer LANGUAGES CXX)

find_package(contract 0.2 CONFIG REQUIRED)

add_executable(contract_package_consumer main.cpp)
target_link_libraries(contract_package_consumer PRIVATE contract::contract)

enable_testing()
add_test(NAME contract_package_consumer_runs COMMAND contract_package_consumer)
