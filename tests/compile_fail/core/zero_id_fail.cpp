// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct ZeroId {
    int value = 0;

    CONTRACT(ZeroId, (value, 0))
};

int main() {
    (void)contract::field_count<ZeroId>();
    return 0;
}
