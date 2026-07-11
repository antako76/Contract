// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary.hpp>

struct PointerRecord {
    int* ptr = nullptr;

    CONTRACT(PointerRecord, (ptr, 1))
};

int main() {
    PointerRecord record;
    unsigned char buffer[sizeof(void*)]{};
    contract::adapters::binary::writer<> out(buffer);
    out << record;
}
