// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Vocabulary {};

struct InvalidAdapter {
    static constexpr contract::adapter_type type = contract::adapter_type::schema;
};

static_assert(!contract::is_vocabulary_visible_v<InvalidAdapter, Vocabulary>);

int main() {
    return 0;
}
