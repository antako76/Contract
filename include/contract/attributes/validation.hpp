#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/resolution.hpp>
#include <contract/definition.hpp>

#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace contract {

enum class attribute_validation_target {
    contract,
    field
};

enum class attribute_validation_issue {
    none,
    unsupported_attribute,
    rejected_requirement
};

struct adapter_validation_summary {
    std::size_t attribute_count = 0;
    std::size_t visible_count = 0;
    std::size_t guarantee_count = 0;
    std::size_t ignored_count = 0;
    std::size_t out_of_scope_count = 0;
    std::size_t unsupported_count = 0;
    std::size_t rejected_count = 0;
    std::size_t error_count = 0;

    [[nodiscard]]
    constexpr bool valid() const noexcept {
        return error_count == 0;
    }
};

template<class AdapterTraits, class Attr>
struct attribute_validation_entry {
    using adapter_traits = AdapterTraits;
    using attribute_type = detail::clean_t<Attr>;

    static constexpr adapter_type adapter = AdapterTraits::type;

    attribute_validation_target target = attribute_validation_target::contract;
    int field_id = 0;
    int declared_field_id = 0;
    int base_offset = 0;
    std::string_view field_name{};
    attribute_type attribute;
    attribute_resolution resolution;
    attribute_validation_issue issue = attribute_validation_issue::none;
};

namespace detail {

struct validation_location {
    attribute_validation_target target;
    int field_id;
    int declared_field_id;
    int base_offset;
    std::string_view field_name;
};

struct ignore_validation_entry {
    template<class Entry>
    constexpr void operator()(const Entry&) const {}
};

template<class Field>
struct declared_field_id : std::integral_constant<int, Field::id> {};

template<class Field, int Offset>
struct declared_field_id<offset_field<Field, Offset>> : declared_field_id<Field> {};

constexpr attribute_validation_issue validation_issue_for(
    attribute_resolution resolution) {
    if (resolution.kind == attribute_resolution_kind::visible &&
        resolution.mode == attribute_mode::error) {
        return attribute_validation_issue::unsupported_attribute;
    }

    if (resolution.kind == attribute_resolution_kind::visible &&
        resolution.mode == attribute_mode::reject) {
        return attribute_validation_issue::rejected_requirement;
    }

    return attribute_validation_issue::none;
}

constexpr bool contributes_guarantee(attribute_resolution resolution) {
    return resolution.kind == attribute_resolution_kind::visible &&
           resolution.mode == attribute_mode::enforce;
}

template<class AdapterTraits, class Attr, class Fn>
constexpr void validate_attribute(
    const Attr& attr,
    validation_location location,
    Fn& fn,
    adapter_validation_summary& summary) {
    const auto resolution = ::contract::resolve_attribute_mode<AdapterTraits>(attr);
    const auto issue = validation_issue_for(resolution);

    ++summary.attribute_count;

    if (resolution.kind != attribute_resolution_kind::invisible) {
        ++summary.visible_count;
    }
    if (contributes_guarantee(resolution)) {
        ++summary.guarantee_count;
    }
    if (resolution.kind == attribute_resolution_kind::visible &&
        resolution.mode == attribute_mode::ignore) {
        ++summary.ignored_count;
    }
    if (resolution.kind == attribute_resolution_kind::visible &&
        resolution.mode == attribute_mode::out_of_scope) {
        ++summary.out_of_scope_count;
    }
    if (issue == attribute_validation_issue::unsupported_attribute) {
        ++summary.unsupported_count;
    }
    if (issue == attribute_validation_issue::rejected_requirement) {
        ++summary.rejected_count;
    }
    if (issue != attribute_validation_issue::none) {
        ++summary.error_count;
    }

    fn(attribute_validation_entry<AdapterTraits, Attr>{
        location.target,
        location.field_id,
        location.declared_field_id,
        location.base_offset,
        location.field_name,
        attr,
        resolution,
        issue
    });
}

template<class AdapterTraits, class... Attrs, class Fn>
constexpr void validate_attributes(
    const attributes<Attrs...>& attrs,
    validation_location location,
    Fn& fn,
    adapter_validation_summary& summary) {
    std::apply(
        [&](const auto&... entries) {
            (validate_attribute<AdapterTraits>(
                entries.attribute,
                location,
                fn,
                summary), ...);
        },
        attrs.entries);
}

template<class AdapterTraits, class Field, class Fn>
constexpr void validate_field(
    const Field& field,
    Fn& fn,
    adapter_validation_summary& summary) {
    constexpr int declared_id = declared_field_id<Field>::value;

    validate_attributes<AdapterTraits>(
        field.attributes,
        validation_location{
            attribute_validation_target::field,
            Field::id,
            declared_id,
            Field::id - declared_id,
            field.name
        },
        fn,
        summary);
}

} // namespace detail

template<class T, class AdapterTraits, class Fn>
constexpr adapter_validation_summary validate_adapter_mode(Fn&& fn) {
    using object_type = detail::clean_t<T>;

    auto definition = contract_of<object_type>();
    auto fields = flattened_fields_of<object_type>();
    adapter_validation_summary summary{};
    auto&& callback = fn;

    detail::validate_attributes<AdapterTraits>(
        definition.attributes,
        detail::validation_location{
            attribute_validation_target::contract,
            0,
            0,
            0,
            {}
        },
        callback,
        summary);

    std::apply(
        [&](const auto&... field) {
            (detail::validate_field<AdapterTraits>(
                field,
                callback,
                summary), ...);
        },
        fields);

    return summary;
}

template<class T, class AdapterTraits>
constexpr adapter_validation_summary validate_adapter_mode() {
    return validate_adapter_mode<T, AdapterTraits>(
        detail::ignore_validation_entry{});
}

template<class T, class AdapterTraits>
inline constexpr bool adapter_mode_is_valid_v =
    validate_adapter_mode<T, AdapterTraits>().valid();

template<class T, class AdapterTraits>
constexpr void require_adapter_mode() {
    static_assert(adapter_mode_is_valid_v<T, AdapterTraits>,
        "CONTRACT adapter mode validation failed");
}

} // namespace contract
