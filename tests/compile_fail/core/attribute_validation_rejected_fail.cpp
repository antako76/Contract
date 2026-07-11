// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>

struct Vocabulary {};
struct RequiredPolicy {};

namespace contract {

template<>
struct attr_traits<RequiredPolicy> {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

struct Event {
    int value = 0;

    CONTRACT(Event, (value, 1, RequiredPolicy{}))
};

struct Adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::wire;
    using visible_vocabularies = contract::vocabularies<Vocabulary>;
};

constexpr auto validate() {
    contract::require_adapter_mode<Event, Adapter>();
}

int main() {
    validate();
    return 0;
}
