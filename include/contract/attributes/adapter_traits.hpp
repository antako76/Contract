#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/attributes.hpp>

#include <type_traits>

namespace contract {

template<class... Vocabularies>
struct vocabularies {};

template<class... Rules>
struct attribute_rules;

enum class attribute_visibility {
    declared_vocabularies_only,
    all_attrs_read_only
};

enum class adapter_type {
    log,
    debug,
    wire,
    storage,
    schema,
    config,
    validation
};

namespace detail {

template<class... Ts>
struct unique_types : std::true_type {};

template<class First, class... Rest>
struct unique_types<First, Rest...>
    : std::bool_constant<
          (!std::is_same_v<First, Rest> && ...) &&
          unique_types<Rest...>::value> {};

template<class T>
struct is_vocabulary_set : std::false_type {};

template<class... Vocabularies>
struct is_vocabulary_set<vocabularies<Vocabularies...>>
    : unique_types<Vocabularies...> {};

template<class Set, class Vocabulary>
struct vocabulary_set_contains : std::false_type {};

template<class... Vocabularies, class Vocabulary>
struct vocabulary_set_contains<vocabularies<Vocabularies...>, Vocabulary>
    : std::bool_constant<(std::is_same_v<Vocabularies, Vocabulary> || ...)> {};

template<class Traits, class = void>
struct adapter_visibility
    : std::integral_constant<attribute_visibility, attribute_visibility::declared_vocabularies_only> {};

template<class Traits>
struct adapter_visibility<Traits, std::void_t<decltype(Traits::visibility)>>
    : std::integral_constant<attribute_visibility, Traits::visibility> {};

template<class Traits, class = void>
struct declared_vocabularies_only {
    using type = vocabularies<>;
    static constexpr bool present = false;
};

template<class Traits>
struct declared_vocabularies_only<Traits, std::void_t<typename Traits::visible_vocabularies>> {
    using type = typename Traits::visible_vocabularies;
    static constexpr bool present = true;
};

} // namespace detail

template<class Traits>
inline constexpr attribute_visibility adapter_visibility_v =
    detail::adapter_visibility<Traits>::value;

template<class Traits>
inline constexpr bool has_all_attributes_read_only_v =
    adapter_visibility_v<Traits> == attribute_visibility::all_attrs_read_only;

template<class Traits>
struct visible_vocabularies {
private:
    using declaration = detail::declared_vocabularies_only<Traits>;

    static_assert(
        has_all_attributes_read_only_v<Traits> || declaration::present,
        "CONTRACT adapter must declare visible_vocabularies or all_attrs_read_only visibility");
    static_assert(detail::is_vocabulary_set<typename declaration::type>::value,
        "CONTRACT visible_vocabularies must be a duplicate-free vocabularies<...> set");

public:
    using type = typename declaration::type;
};

template<class Traits>
using visible_vocabularies_t = typename visible_vocabularies<Traits>::type;

template<class Traits, class Vocabulary>
struct is_vocabulary_visible
    : std::bool_constant<
          has_all_attributes_read_only_v<Traits> ||
          detail::vocabulary_set_contains<
              visible_vocabularies_t<Traits>,
              Vocabulary>::value> {};

template<class Traits, class Vocabulary>
inline constexpr bool is_vocabulary_visible_v =
    is_vocabulary_visible<Traits, Vocabulary>::value;

template<class Traits, class Attr>
struct is_attribute_visible
    : is_vocabulary_visible<
          Traits,
          typename attr_traits<detail::clean_t<Attr>>::vocabulary> {};

template<class Traits, class Attr>
inline constexpr bool is_attribute_visible_v =
    is_attribute_visible<Traits, Attr>::value;

} // namespace contract
