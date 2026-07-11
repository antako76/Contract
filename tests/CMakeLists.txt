set(CONTRACT_TESTS_DIR ${CMAKE_CURRENT_LIST_DIR})

function(add_contract_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE contract::contract)
    target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
    # Tests assert their way to failure; keep assert() live even when the rest
    # of the project is configured as Release (which defines NDEBUG).
    target_compile_options(${name} PRIVATE -UNDEBUG)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endfunction()

function(add_contract_compile_fail_test name source)
    add_test(
        NAME ${name}
        COMMAND
            ${CMAKE_COMMAND}
            -DTEST_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/${source}
            -DTEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}
            -DCONTRACT_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
            -DCONTRACT_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCONTRACT_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCONTRACT_ENABLE_SPAN=${CONTRACT_ENABLE_SPAN}
            -P ${CONTRACT_TESTS_DIR}/expect_compile_fail.cmake
    )
endfunction()

add_subdirectory(unit)
add_subdirectory(compile_fail)
