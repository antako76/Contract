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

constexpr auto invalid = contract::compose_attributes(
    contract::make_attributes(contract::describe_attribute(Unique{}, "Unique{}")),
    contract::make_attributes(contract::describe_attribute(Unique{}, "Unique{}")));

int main() {
    return 0;
}
