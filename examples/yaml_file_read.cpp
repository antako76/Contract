// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/yaml/all.hpp>
#include <contract/contract.hpp>
#include <contract/cout.hpp>
#include <contract/io.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct PaymentConfig {
    std::string service;
    std::uint32_t port = 0;
    bool enabled = false;
    std::vector<std::string> tags;

    CONTRACT(PaymentConfig,
        (service, 1),
        (port, 2),
        (enabled, 3),
        (tags, 4))
};

int main(int argc, char** argv) {
    std::string path;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 1) {
        path = std::string(CONTRACT_EXAMPLE_SOURCE_DIR) + "/yaml/payment_config.yaml";
    } else {
        std::cerr << "usage: " << argv[0] << " [yaml-file]\n";
        return 1;
    }

    PaymentConfig config{};
    try {
        contract::adapters::yaml::reader<contract::io::file_buffer_input> in(
            contract::io::file_buffer_input{path});
        in >> config;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    contract::cout.debug() << config;
    std::cout << '\n';

    return 0;
}
