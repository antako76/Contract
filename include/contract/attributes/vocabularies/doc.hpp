#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/adapter_traits.hpp>

#include <string_view>

namespace contract::doc {

struct vocabulary {};

struct comment {
    std::string_view text;

    constexpr explicit comment(std::string_view text)
        : text(text) {}
};

} // namespace contract::doc

namespace contract {

template<>
struct attr_traits<doc::comment> {
    using vocabulary = doc::vocabulary;
    static constexpr attr_targets targets{true, true, false, false, false};
    static constexpr bool repeatable = true;
};

} // namespace contract
