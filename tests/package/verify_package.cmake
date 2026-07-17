# Copyright 2026 Ilya Korolev
# Licensed under the Apache License, Version 2.0
# SPDX-License-Identifier: Apache-2.0

file(REMOVE_RECURSE
    "${CONTRACT_PACKAGE_TEST_BINARY_DIR}"
    "${CONTRACT_PACKAGE_TEST_PREFIX}"
)

set(config_args)
set(ctest_config_args)
if(NOT CONTRACT_BUILD_CONFIG STREQUAL "")
    list(APPEND config_args --config "${CONTRACT_BUILD_CONFIG}")
    list(APPEND ctest_config_args -C "${CONTRACT_BUILD_CONFIG}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" --install "${CONTRACT_BINARY_DIR}"
        --prefix "${CONTRACT_PACKAGE_TEST_PREFIX}"
        ${config_args}
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR
        "CONTRACT package installation failed:\n${install_output}${install_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        -S "${CONTRACT_PACKAGE_TEST_SOURCE_DIR}"
        -B "${CONTRACT_PACKAGE_TEST_BINARY_DIR}"
        -G "${CONTRACT_CMAKE_GENERATOR}"
        "-DCMAKE_CXX_COMPILER=${CONTRACT_CXX_COMPILER}"
        "-DCMAKE_PREFIX_PATH=${CONTRACT_PACKAGE_TEST_PREFIX}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "CONTRACT package consumer configuration failed:\n${configure_output}${configure_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" --build "${CONTRACT_PACKAGE_TEST_BINARY_DIR}"
        ${config_args}
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR
        "CONTRACT package consumer build failed:\n${build_output}${build_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_CTEST_COMMAND}" --test-dir "${CONTRACT_PACKAGE_TEST_BINARY_DIR}"
        ${ctest_config_args}
        --output-on-failure
    RESULT_VARIABLE test_result
    OUTPUT_VARIABLE test_output
    ERROR_VARIABLE test_error
)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR
        "CONTRACT package consumer test failed:\n${test_output}${test_error}"
    )
endif()
