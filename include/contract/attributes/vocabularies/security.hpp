#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/adapter_traits.hpp>

namespace contract::security {

struct vocabulary {};

struct sensitive {};
struct secret {};
struct no_log {};
struct encrypt {};

} // namespace contract::security

namespace contract {

template<>
struct attr_traits<security::sensitive> {
    using vocabulary = contract::security::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<security::secret> {
    using vocabulary = contract::security::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<security::no_log> {
    using vocabulary = contract::security::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<security::encrypt> {
    using vocabulary = contract::security::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract
