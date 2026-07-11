// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct Vocabulary {};
struct Unique {};

namespace contract {

template<>
struct attr_traits<Unique> {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

struct DuplicateAttribute {
    int value = 0;

    CONTRACT(DuplicateAttribute, (value, 1, Unique{}, Unique{}))
};

int main() {
    (void)contract::contract_of<DuplicateAttribute>();
    return 0;
}
