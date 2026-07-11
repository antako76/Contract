#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/definition.hpp>

#include <tuple>
#include <type_traits>
#include <utility>

namespace contract {

template<class T, class Fn>
constexpr void for_each_field(Fn&& fn) {
    using object_type = std::remove_cvref_t<T>;
    auto fields = flattened_fields_of<object_type>();

    std::apply(
        [&](const auto&... fields) {
            std::forward<Fn>(fn)(fields...);
        },
        fields);
}

template<class T>
constexpr auto field_count() {
    using object_type = std::remove_cvref_t<T>;
    return std::tuple_size<decltype(flattened_fields_of<object_type>())>::value;
}

template<std::size_t Index, class T>
[[nodiscard]]
constexpr auto field_at() {
    using object_type = std::remove_cvref_t<T>;
    auto fields = flattened_fields_of<object_type>();
    return std::get<Index>(fields);
}

template<class Object, class Adapter>
constexpr void visit(Object& obj, Adapter&& adapter) {
    using object_type = std::remove_cvref_t<Object>;
    auto fields = flattened_fields_of<object_type>();

    std::apply(
        [&](const auto&... fields) {
            (adapter.field(fields, obj), ...);
        },
        fields);
}

template<class Object, class Adapter>
constexpr void visit(const Object& obj, Adapter&& adapter) {
    using object_type = std::remove_cvref_t<Object>;
    auto fields = flattened_fields_of<object_type>();

    std::apply(
        [&](const auto&... fields) {
            (adapter.field(fields, obj), ...);
        },
        fields);
}

template<class Object, class Adapter>
constexpr void visit(Object&&, Adapter&&) = delete;

} // namespace contract
