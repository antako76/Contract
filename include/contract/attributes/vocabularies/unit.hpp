#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/adapter_traits.hpp>

#include <string_view>

namespace contract::unit {

struct vocabulary {};

struct ucum {
    std::string_view symbol;

    constexpr explicit ucum(std::string_view symbol)
        : symbol(symbol) {}
};

constexpr auto seconds() {
    return ucum{"s"};
}

constexpr auto milliseconds() {
    return ucum{"ms"};
}

constexpr auto microseconds() {
    return ucum{"us"};
}

constexpr auto nanoseconds() {
    return ucum{"ns"};
}

constexpr auto unix_seconds() {
    return ucum{"s"};
}

constexpr auto unix_milliseconds() {
    return ucum{"ms"};
}

} // namespace contract::unit

namespace contract {

template<>
struct attr_traits<unit::ucum> {
    using vocabulary = unit::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract
