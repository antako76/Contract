// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace attribute_tests {

struct test_vocabulary {};
struct other_vocabulary {};

struct type_code {
    int value;
};

struct field_code {
    int value;
};

struct label {
    const char* value;
};

struct other {
    int value;
};

} // namespace attribute_tests

namespace contract {

template<>
struct attr_traits<attribute_tests::type_code> {
    using vocabulary = attribute_tests::test_vocabulary;
    static constexpr attr_targets targets{true, false, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<attribute_tests::field_code> {
    using vocabulary = attribute_tests::test_vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<attribute_tests::label> {
    using vocabulary = attribute_tests::test_vocabulary;
    static constexpr attr_targets targets{true, true, false, false, false};
    static constexpr bool repeatable = true;
};

template<>
struct attr_traits<attribute_tests::other> {
    using vocabulary = attribute_tests::other_vocabulary;
    static constexpr attr_targets targets{true, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

namespace attribute_tests {

struct Base {
    std::uint32_t id = 0;

    CONTRACT(Base,
        ATTRS(type_code{10}),
        (id, 1, field_code{20}, label{"base-id"}))
};

struct Middle : Base {
    CONTRACT(Middle, BASE(Base, 100))
};

struct Event : Middle {
    std::uint32_t raw_value = 0;

    CONTRACT(Event,
        ATTRS(type_code{30}, label{"event"}),
        BASE(Middle, 1000),
        PROPERTY(value, 2, std::uint32_t,
            field_code{40}, other{50}, label{"property"}, label{"display"}))

    std::uint32_t contract_get(contract::tag<contract_field::value>) const {
        return raw_value;
    }

    void contract_set(contract::tag<contract_field::value>, std::uint32_t value) {
        raw_value = value;
    }
};

} // namespace attribute_tests

int main() {
    using namespace attribute_tests;

    constexpr auto base_attributes = contract::contract_attributes_of<Base>();
    static_assert(std::tuple_size_v<typename decltype(base_attributes)::tuple_type> == 1);
    static_assert(std::get<0>(base_attributes.entries).attribute.value == 10);
    static_assert(std::get<0>(base_attributes.entries).source ==
        std::string_view("type_code{10}"));

    constexpr auto event_attributes = contract::contract_attributes_of<Event>();
    static_assert(std::tuple_size_v<typename decltype(event_attributes)::tuple_type> == 2);
    static_assert(std::get<0>(event_attributes.entries).attribute.value == 30);
    static_assert(std::get<1>(event_attributes.entries).attribute.value[0] == 'e');

    constexpr auto base_field = contract::field_at<0, Event>();
    using base_field_type = decltype(base_field);
    static_assert(base_field.id == 1101);
    static_assert(base_field.is_base_import);
    static_assert(contract::has_attribute_v<base_field_type, field_code>);
    static_assert(contract::has_attribute_v<base_field_type, label>);
    static_assert(!contract::has_attribute_v<base_field_type, type_code>);
    static_assert(contract::attribute_count_v<base_field_type, label> == 1);
    static_assert(std::get<0>(base_field.attributes.entries).attribute.value == 20);
    static_assert(std::get<1>(base_field.attributes.entries).attribute.value[0] == 'b');

    constexpr auto property = contract::field_at<1, Event>();
    using property_type = decltype(property);
    static_assert(property.id == 2);
    static_assert(property.is_property_field);
    static_assert(std::is_same_v<
        contract::attributes_of_t<property_type>,
        contract::attributes<field_code, other, label, label>>);
    static_assert(contract::attribute_count_v<property_type, label> == 2);
    static_assert(std::is_same_v<
        contract::attributes_in_vocabulary_t<property_type, test_vocabulary>,
        contract::attributes<field_code, label, label>>);
    static_assert(std::get<0>(property.attributes.entries).attribute.value == 40);
    static_assert(std::get<1>(property.attributes.entries).attribute.value == 50);
    static_assert(std::get<2>(property.attributes.entries).attribute.value[0] == 'p');
    static_assert(std::get<0>(property.attributes.entries).source ==
        std::string_view("field_code{40}"));
    static_assert(property.attributes.find<field_code>()->value == 40);
    static_assert(property.attributes.get<field_code>().value == 40);
    static_assert(property.attributes.find<type_code>() == nullptr);

    constexpr auto filtered =
        contract::get_attributes_in_vocabulary<test_vocabulary>(property);
    static_assert(std::get<0>(filtered.entries).attribute.value == 40);
    static_assert(std::get<1>(filtered.entries).attribute.value[0] == 'p');
    static_assert(std::get<2>(filtered.entries).attribute.value[0] == 'd');

    constexpr auto composed = contract::compose_attributes(
        contract::make_attributes(
            contract::describe_attribute(field_code{60}, "field_code{60}"),
            contract::describe_attribute(label{"left"}, "label{\"left\"}")),
        contract::make_attributes(
            contract::describe_attribute(other{70}, "other{70}"),
            contract::describe_attribute(label{"right"}, "label{\"right\"}")));
    static_assert(std::get<0>(composed.entries).attribute.value == 60);
    static_assert(std::get<1>(composed.entries).attribute.value[0] == 'l');
    static_assert(std::get<2>(composed.entries).attribute.value == 70);
    static_assert(std::get<3>(composed.entries).attribute.value[0] == 'r');

    const auto& declared = contract::get_attributes(property);
    static_assert(std::is_same_v<
        std::remove_cv_t<std::remove_reference_t<decltype(declared)>>,
        contract::attributes_of_t<property_type>>);
    assert(&declared == &property.attributes);

    Event event;
    property.set(event, 55);
    assert(property.get(event) == 55);

    return 0;
}
