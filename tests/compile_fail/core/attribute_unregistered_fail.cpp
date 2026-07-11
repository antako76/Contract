// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Unregistered {};

struct UnknownAttribute {
    int value = 0;

    CONTRACT(UnknownAttribute, (value, 1, Unregistered{}))
};

int main() {
    (void)contract::contract_of<UnknownAttribute>();
    return 0;
}
