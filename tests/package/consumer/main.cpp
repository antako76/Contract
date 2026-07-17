// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct package_message {
    int value = 42;

    CONTRACT(package_message, (value, 1))
};

int main() {
    package_message message;
    return message.value == 42 ? 0 : 1;
}
