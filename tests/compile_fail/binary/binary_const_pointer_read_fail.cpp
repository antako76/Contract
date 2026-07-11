// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary.hpp>

struct ConstPointerRecord {
    const char* text = nullptr;

    CONTRACT(ConstPointerRecord, (text, 1))
};

int main() {
    ConstPointerRecord record;
    unsigned char buffer[8]{};
    contract::adapters::binary::reader<> in(buffer);
    in >> record;
}
