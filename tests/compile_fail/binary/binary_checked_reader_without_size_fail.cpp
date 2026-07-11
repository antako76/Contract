// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary.hpp>

int main() {
    unsigned char buffer[8]{};
    contract::adapters::binary::reader<contract::io::checked_input> in(buffer);
    (void)in;
}
