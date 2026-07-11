#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/adapter_traits.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace contract::schema {

struct vocabulary {};

struct string_type {};
struct bytes_type {};
struct fixed32_type {};
struct fixed64_type {};

inline constexpr string_type string{};
inline constexpr bytes_type bytes{};
inline constexpr fixed32_type fixed32{};
inline constexpr fixed64_type fixed64{};

template<class LogicalType>
struct type {
    using logical_type = LogicalType;

    constexpr explicit type(LogicalType) {}
};

template<class LogicalType>
type(LogicalType) -> type<std::decay_t<LogicalType>>;

struct reserved {
    std::string_view name;
    std::size_t id;

    constexpr reserved(std::string_view name, std::size_t id)
        : name(name), id(id) {}
};

struct reserved_id {
    std::size_t id;

    constexpr explicit reserved_id(std::size_t id)
        : id(id) {}
};

struct reserved_name {
    std::string_view name;

    constexpr explicit reserved_name(std::string_view name)
        : name(name) {}
};

struct reserved_range {
    std::size_t first;
    std::size_t last;

    constexpr reserved_range(std::size_t first, std::size_t last)
        : first(first), last(last) {}
};

struct deprecated {};

struct alias {
    std::string_view name;

    constexpr explicit alias(std::string_view name)
        : name(name) {}
};

} // namespace contract::schema

namespace contract {

template<class LogicalType>
struct attr_traits<schema::type<LogicalType>> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<schema::reserved> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = true;
};

template<>
struct attr_traits<schema::reserved_id> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = true;
};

template<>
struct attr_traits<schema::reserved_name> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = true;
};

template<>
struct attr_traits<schema::reserved_range> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = true;
};

template<>
struct attr_traits<schema::deprecated> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<schema::alias> {
    using vocabulary = schema::vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = true;
};

} // namespace contract
