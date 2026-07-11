// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Vocabulary {};
struct FieldOnly {};

namespace contract {

template<>
struct attr_traits<FieldOnly> {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

struct InvalidContractTarget {
    int value = 0;

    CONTRACT(InvalidContractTarget, ATTRS(FieldOnly{}), (value, 1))
};

int main() {
    (void)contract::contract_of<InvalidContractTarget>();
    return 0;
}
