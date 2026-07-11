add_executable(contract_binary_adapter_benchmark binary_adapter_benchmark.cpp)
target_link_libraries(contract_binary_adapter_benchmark PRIVATE contract::contract)

add_executable(contract_protobuf_binary_simple_benchmark protobuf_binary_simple_benchmark.cpp)
target_link_libraries(contract_protobuf_binary_simple_benchmark PRIVATE contract::contract)

add_executable(contract_protobuf_compact_benchmark protobuf_compact_adapter_benchmark.cpp)
target_link_libraries(contract_protobuf_compact_benchmark PRIVATE contract::contract)

option(CONTRACT_BENCH_WITH_PROTOBUF "Also benchmark CONTRACT's protobuf adapter against real libprotobuf (requires protoc + libprotobuf-dev)" OFF)

if(CONTRACT_BENCH_WITH_PROTOBUF)
    find_package(Protobuf REQUIRED)
    protobuf_generate_cpp(CONTRACT_BENCH_PROTO_SRCS CONTRACT_BENCH_PROTO_HDRS protobuf_reference_messages.proto)

    add_executable(contract_protobuf_reference_benchmark
        protobuf_reference_benchmark.cpp
        ${CONTRACT_BENCH_PROTO_SRCS}
    )
    target_include_directories(contract_protobuf_reference_benchmark PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(contract_protobuf_reference_benchmark PRIVATE contract::contract protobuf::libprotobuf)
endif()
