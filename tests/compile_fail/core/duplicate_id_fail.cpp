// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct DuplicateId {
    int first = 0;
    int second = 0;

    CONTRACT(DuplicateId, (first, 1), (second, 1))
};

int main() {
    (void)contract::field_count<DuplicateId>();
    return 0;
}
