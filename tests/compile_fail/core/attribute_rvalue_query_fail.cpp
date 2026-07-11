// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Value {
    int field = 0;

    CONTRACT(Value, (field, 1))
};

constexpr auto invalid = contract::get_attributes(contract::field_at<0, Value>());

int main() {
    return 0;
}
