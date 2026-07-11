// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Vocabulary {};
struct TypeOnly {};

namespace contract {

template<>
struct attr_traits<TypeOnly> {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

struct InvalidFieldTarget {
    int value = 0;

    CONTRACT(InvalidFieldTarget, (value, 1, TypeOnly{}))
};

int main() {
    (void)contract::contract_of<InvalidFieldTarget>();
    return 0;
}
