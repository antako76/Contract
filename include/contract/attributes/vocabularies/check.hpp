#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/adapter_traits.hpp>

#include <cstddef>

namespace contract::check {

struct vocabulary {};

enum class tag {
    constraint,
    decode_guard,
    value,
    length,
    bytes,
    items,
    presence,
    pattern
};

template<auto... Tags>
using tag_set = tags<Tags...>;

template<class T>
struct min_value {
    T value;

    constexpr explicit min_value(T value)
        : value(value) {}
};

template<class T>
min_value(T) -> min_value<T>;

template<class T>
struct max_value {
    T value;

    constexpr explicit max_value(T value)
        : value(value) {}
};

template<class T>
max_value(T) -> max_value<T>;

struct min_length {
    std::size_t value;

    constexpr explicit min_length(std::size_t value)
        : value(value) {}
};

struct max_length {
    std::size_t value;

    constexpr explicit max_length(std::size_t value)
        : value(value) {}
};

struct max_bytes {
    std::size_t value;

    constexpr explicit max_bytes(std::size_t value)
        : value(value) {}
};

struct max_items {
    std::size_t value;

    constexpr explicit max_items(std::size_t value)
        : value(value) {}
};

struct not_empty {};
template<class Attr, tag Tag>
inline constexpr bool has_tag_v = has_attribute_tag_v<Attr, Tag>;

template<class Attr>
inline constexpr bool has_decode_guard_v = has_tag_v<Attr, tag::decode_guard>;

template<class Attr>
inline constexpr bool is_constraint_v = has_tag_v<Attr, tag::constraint>;

template<class Attr>
inline constexpr bool is_presence_v = has_tag_v<Attr, tag::presence>;

template<class Attr>
inline constexpr bool is_value_v = has_tag_v<Attr, tag::value>;

} // namespace contract::check

namespace contract {

template<class T>
struct attr_traits<check::min_value<T>> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<check::tag::constraint, check::tag::value>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<class T>
struct attr_traits<check::max_value<T>> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<check::tag::constraint, check::tag::value>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<check::min_length> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<check::tag::constraint, check::tag::length>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<check::max_length> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<
        check::tag::constraint,
        check::tag::length,
        check::tag::decode_guard>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<check::max_bytes> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<
        check::tag::constraint,
        check::tag::bytes,
        check::tag::decode_guard>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<check::max_items> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<
        check::tag::constraint,
        check::tag::items,
        check::tag::decode_guard>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<check::not_empty> {
    using vocabulary = check::vocabulary;
    using tags = contract::tags<
        check::tag::constraint,
        check::tag::presence>;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract
