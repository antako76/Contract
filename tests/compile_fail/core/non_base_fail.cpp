// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct NotBase {
    int value = 0;

    CONTRACT(NotBase, (value, 1))
};

struct Owner {
    int own = 0;

    CONTRACT(Owner, BASE(NotBase, 1000), (own, 2001))
};

int main() {
    (void)contract::field_count<Owner>();
    return 0;
}
