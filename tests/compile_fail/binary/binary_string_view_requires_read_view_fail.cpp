// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary.hpp>

#include <cstddef>
#include <cstring>
#include <string_view>

struct read_only_input {
    explicit read_only_input(const unsigned char* data)
        : current(data) {}

    void read(void* out, std::size_t size) {
        std::memcpy(out, current, size);
        current += size;
    }

    const unsigned char* current;
};

int main() {
    unsigned char buffer[32]{};
    contract::adapters::binary::reader<read_only_input> in(read_only_input{buffer});

    std::string_view value;
    in >> value;
}
