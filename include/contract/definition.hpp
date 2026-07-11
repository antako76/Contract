#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/attributes.hpp>
#include <contract/detail/validation.hpp>
#include <contract/field.hpp>
#include <contract/tag.hpp>

#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace contract {

template<class Owner, class Attributes, class... Fields>
struct definition {
    using owner_type = Owner;
    using attributes_type = Attributes;
    using fields_type = std::tuple<Fields...>;

    // This is the canonical core descriptor returned by contract_definition(tag<T>).
    std::string_view type_name;
    attributes_type attributes;
    fields_type fields;
};

template<class Owner, class... Fields>
constexpr auto make_contract(std::string_view type_name, Fields... fields) {
    using attributes_type = attributes<>;
    using definition_type = definition<Owner, attributes_type, Fields...>;
    return definition_type{
        type_name,
        attributes_type{},
        typename definition_type::fields_type{fields...}
    };
}

template<class Owner, class... Attrs, class... Fields>
constexpr auto make_contract_with_attributes(
    std::string_view type_name,
    attributes<Attrs...> attrs,
    Fields... fields) {
    using attributes_type = attributes<Attrs...>;
    using definition_type = definition<Owner, attributes_type, Fields...>;
    return definition_type{
        type_name,
        std::move(attrs),
        typename definition_type::fields_type{fields...}
    };
}

template<class Owner, class Entry>
constexpr auto flatten_entry(Entry entry);

template<class Owner, class Definition>
constexpr auto flatten_definition(Definition def);

template<class T>
constexpr auto contract_of() {
    using clean_type = std::remove_cvref_t<T>;
    // contract_definition(tag<T>) is the SSOT for a type's descriptor.
    auto def = contract_definition(tag<clean_type>{});

    static_assert(std::is_same_v<typename decltype(def)::owner_type, clean_type>,
        "contract_definition(tag<T>) must return contract::definition<T, ...>");

    return def;
}

template<class T>
constexpr std::string_view type_name() {
    using clean_type = std::remove_cvref_t<T>;
    return contract_of<clean_type>().type_name;
}

template<class T>
constexpr auto contract_attributes_of() {
    return contract_of<T>().attributes;
}

template<class Owner, class Entry>
constexpr auto flatten_entry(Entry entry) {
    // Ordinary entries pass through unchanged.
    return std::make_tuple(entry);
}

// BASE entries are expanded into ordinary adapter-facing fields. This keeps
// offset handling in the core instead of duplicating it in every adapter.
template<class Owner, class Base, int Offset>
constexpr auto flatten_entry(base<Base, Offset>) {
    detail::check_base_entry<Owner, Base>();

    // BASE imports are flattened into effective fields with adjusted ids.
    auto base_fields = flatten_definition<Base>(contract_of<Base>());

    return std::apply(
        [](const auto&... fields) {
            return std::make_tuple(offset_field<std::remove_cv_t<std::remove_reference_t<decltype(fields)>>, Offset>{fields}...);
        },
        base_fields);
}

template<class Owner, class Definition>
constexpr auto flatten_definition(Definition def) {
    // Flatten the declared entry list into the adapter-facing field tuple.
    return std::apply(
        [](const auto&... entries) {
            return std::tuple_cat(flatten_entry<Owner>(entries)...);
        },
        def.fields);
}

template<class T>
constexpr auto flattened_fields_of() {
    using clean_type = std::remove_cvref_t<T>;
    auto fields = flatten_definition<clean_type>(contract_of<clean_type>());

    // Validate the final adapter-facing tuple after BASE expansion.
    detail::check_flattened_tuple<decltype(fields)>::apply();

    return fields;
}

} // namespace contract
