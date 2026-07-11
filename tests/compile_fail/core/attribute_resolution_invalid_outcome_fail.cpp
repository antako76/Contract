// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>

struct Vocabulary {};
struct Policy {};

namespace contract {

template<>
struct attr_traits<Policy> {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

struct InvalidAdapter {
    static constexpr contract::adapter_type type = contract::adapter_type::log;
    using visible_vocabularies = contract::vocabularies<Vocabulary>;
    using attribute_rules = contract::attribute_rules<
        contract::for_attr<Policy>::ignore,
        contract::for_attr<Policy>::enforce>;
};

constexpr auto result =
    contract::resolve_attribute_mode<InvalidAdapter>(Policy{});

int main() {
    return static_cast<int>(result.kind);
}
