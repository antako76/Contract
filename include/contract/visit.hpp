#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/definition.hpp>

#include <cstdint>
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

namespace detail {

// A fold expression over one id compare per field, not a recursive chain:
// the compiler can turn this into a jump table for dense ids, the same way a
// literal switch would, without macro-generated case labels (which can't
// cross a BASE import's type boundary). Works uniformly across BASE because
// it walks the already-flattened field tuple (effective ids), not raw
// entries. fn must be called from within this same fold, at the point of the
// match - resolving the index first and acting on it in a second pass
// defeats the fusion into one dispatch and each pass degrades to a plain
// compare chain. always_inline: without it the compiler does not fold this
// into a caller's loop for wider messages.
template<class T, class Fn, std::size_t... Is>
[[gnu::always_inline]] constexpr bool dispatch_field_by_id_impl(
    std::uint32_t id, Fn& fn, std::index_sequence<Is...>) {
    bool found = false;
    auto try_field = [&]<std::size_t Index>() {
        if (found || static_cast<std::uint32_t>(field_at<Index, T>().id) != id) {
            return;
        }
        fn(field_at<Index, T>());
        found = true;
    };
    (try_field.template operator()<Is>(), ...);
    return found;
}

} // namespace detail

// Calls fn(field) for the declared field whose effective id matches `id` and
// returns true, or returns false without calling fn if none matches. Callers
// own everything about the not-found case (e.g. constructing an
// adapter-specific error) - this utility only finds and invokes.
template<class T, class Fn>
[[gnu::always_inline]] constexpr bool dispatch_field_by_id(std::uint32_t id, Fn&& fn) {
    using object_type = std::remove_cvref_t<T>;
    return detail::dispatch_field_by_id_impl<object_type>(
        id, fn, std::make_index_sequence<field_count<object_type>()>{});
}

} // namespace contract
