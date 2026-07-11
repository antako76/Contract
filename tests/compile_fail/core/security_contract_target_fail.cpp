// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/security.hpp>

struct InvalidSecurityContract {
    CONTRACT(InvalidSecurityContract,
        ATTRS(contract::security::secret()))
};

int main() {
    return 0;
}
