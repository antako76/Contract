#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <tuple>
#include <type_traits>

namespace contract::detail {

template<class... Fields>
struct unique_field_ids : std::true_type {};

template<class First, class... Rest>
struct unique_field_ids<First, Rest...>
    : std::bool_constant<((First::id != Rest::id) && ...) && unique_field_ids<Rest...>::value> {};

template<class... Fields>
constexpr void check_flattened_fields() {
    static_assert(((Fields::id > 0) && ...),
        "CONTRACT field ids must be positive");
    static_assert(unique_field_ids<Fields...>::value,
        "CONTRACT field ids must be unique after BASE offsets are applied");
}

template<class Owner, class Base>
constexpr void check_base_entry() {
    static_assert(!std::is_same_v<Base, Owner>,
        "CONTRACT BASE(Type, offset): Type must not be the owner type itself");
    static_assert(std::is_base_of_v<Base, Owner>,
        "CONTRACT BASE(Type, offset): Type must be a base class of owner");
    static_assert(std::is_convertible_v<Owner*, Base*>,
        "CONTRACT BASE(Type, offset): Type must be an accessible and unambiguous base class");
}

template<class Tuple>
struct check_flattened_tuple;

template<class... Fields>
struct check_flattened_tuple<std::tuple<Fields...>> {
    static constexpr void apply() {
        check_flattened_fields<Fields...>();
    }
};

} // namespace contract::detail
