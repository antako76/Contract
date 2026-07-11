// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>
#include <contract/check.hpp>
#include <contract/doc.hpp>
#include <contract/schema.hpp>
#include <contract/unit.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

struct CommonRecord {
    std::int64_t id = 0;
    std::string_view name{};

    CONTRACT(CommonRecord,
        ATTRS(
            contract::schema::reserved_id(7),
            contract::schema::reserved_name("legacy_name"),
            contract::schema::reserved_range(20, 29),
            contract::doc::comment("common record")),
        (id, 1,
            contract::schema::type(contract::schema::fixed64),
            contract::check::max_value(1000),
            contract::unit::seconds(),
            contract::doc::comment("identifier")),
        (name, 2,
            contract::schema::type(contract::schema::string),
            contract::check::max_length(64),
            contract::doc::comment("name")))
};

struct CommonVocabularyAdapter {
    static constexpr contract::adapter_type type = contract::adapter_type::schema;
    static constexpr contract::attribute_visibility visibility =
        contract::attribute_visibility::all_attrs_read_only;
};

struct CheckTagAdapter {
    static constexpr contract::adapter_type type = contract::adapter_type::debug;

    using visible_vocabularies = contract::vocabularies<contract::check::vocabulary>;
    using attribute_rules = contract::attribute_rules<
        contract::for_tag<contract::check::tag::decode_guard>::enforce,
        contract::for_tag<contract::check::tag::presence>::display>;
};

int main() {
    using field0_type = std::remove_cv_t<std::remove_reference_t<decltype(contract::field_at<0, CommonRecord>())>>;
    using field1_type = std::remove_cv_t<std::remove_reference_t<decltype(contract::field_at<1, CommonRecord>())>>;

    static_assert(contract::has_attribute_v<field0_type, contract::schema::type<contract::schema::fixed64_type>>);
    static_assert(contract::has_attribute_v<field0_type, contract::check::max_value<int>>);
    static_assert(contract::has_attribute_v<field0_type, contract::unit::ucum>);
    static_assert(contract::has_attribute_v<field0_type, contract::doc::comment>);
    static_assert(contract::check::has_tag_v<contract::check::max_value<int>, contract::check::tag::constraint>);
    static_assert(contract::check::has_tag_v<contract::check::max_value<int>, contract::check::tag::value>);
    static_assert(contract::check::has_tag_v<contract::check::max_length, contract::check::tag::decode_guard>);
    static_assert(contract::check::has_tag_v<contract::check::max_bytes, contract::check::tag::decode_guard>);
    static_assert(contract::check::has_tag_v<contract::check::max_items, contract::check::tag::decode_guard>);
    static_assert(contract::check::has_tag_v<contract::check::not_empty, contract::check::tag::presence>);
    static_assert(!contract::check::has_tag_v<contract::check::min_value<int>, contract::check::tag::decode_guard>);
    static_assert(contract::has_attribute_tag_v<contract::check::max_length, contract::check::tag::decode_guard>);
    static_assert(contract::resolve_attribute_mode<CheckTagAdapter>(contract::check::max_length(64)).mode ==
        contract::attribute_mode::enforce);
    static_assert(contract::resolve_attribute_mode<CheckTagAdapter>(contract::check::not_empty()).mode ==
        contract::attribute_mode::display);
    static_assert(contract::resolve_attribute_mode<CheckTagAdapter>(contract::check::min_value(1)).mode ==
        contract::attribute_mode::error);

    static_assert(contract::has_attribute_v<field1_type, contract::schema::type<contract::schema::string_type>>);
    static_assert(contract::has_attribute_v<field1_type, contract::check::max_length>);
    static_assert(contract::has_attribute_v<field1_type, contract::doc::comment>);

    static_assert(contract::is_attribute_visible_v<CommonVocabularyAdapter, contract::schema::type<contract::schema::fixed64_type>>);
    static_assert(contract::is_attribute_visible_v<CommonVocabularyAdapter, contract::check::max_length>);
    static_assert(contract::is_attribute_visible_v<CommonVocabularyAdapter, contract::unit::ucum>);
    static_assert(contract::is_attribute_visible_v<CommonVocabularyAdapter, contract::doc::comment>);

    constexpr auto field0 = contract::field_at<0, CommonRecord>();
    static_assert(field0.attributes.get<contract::check::max_value<int>>().value == 1000);

    constexpr auto reserved =
        contract::contract_attributes_of<CommonRecord>();
    static_assert(std::tuple_size_v<typename decltype(reserved)::tuple_type> == 4);
    static_assert(std::get<0>(reserved.entries).attribute.id == 7);
    static_assert(std::get<1>(reserved.entries).attribute.name == std::string_view("legacy_name"));
    static_assert(std::get<2>(reserved.entries).attribute.first == 20);
    static_assert(std::get<2>(reserved.entries).attribute.last == 29);
    static_assert(std::get<3>(reserved.entries).attribute.text[0] == 'c');
    static_assert(std::get<0>(reserved.entries).source ==
        std::string_view("contract::schema::reserved_id(7)"));

    return 0;
}
