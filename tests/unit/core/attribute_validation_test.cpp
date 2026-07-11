// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace attribute_validation_tests {

struct vocabulary {};
struct hidden_vocabulary {};

struct documented {};
struct enforced {};
struct ignored {};
struct unsupported {};
struct hidden {};
struct implemented {};
struct external {};
struct storage_only {};
struct out_of_scope_policy {};

} // namespace attribute_validation_tests

namespace contract {

using attribute_validation_tests::documented;
using attribute_validation_tests::enforced;
using attribute_validation_tests::ignored;
using attribute_validation_tests::unsupported;
using attribute_validation_tests::hidden;
using attribute_validation_tests::implemented;
using attribute_validation_tests::external;
using attribute_validation_tests::storage_only;
using attribute_validation_tests::out_of_scope_policy;
using attribute_validation_tests::vocabulary;
using attribute_validation_tests::hidden_vocabulary;

template<class Vocabulary>
struct validation_test_attr_traits {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{true, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<attribute_validation_tests::documented>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::enforced>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::ignored>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::unsupported>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::hidden>
    : validation_test_attr_traits<attribute_validation_tests::hidden_vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::implemented>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::external>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::storage_only>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

template<>
struct attr_traits<attribute_validation_tests::out_of_scope_policy>
    : validation_test_attr_traits<attribute_validation_tests::vocabulary> {};

} // namespace contract

namespace attribute_validation_tests {

struct log_adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::log;
    using visible_vocabularies = contract::vocabularies<vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::for_attr<documented>::display,
        contract::for_attr<enforced>::enforce,
        contract::for_attr<ignored>::ignore,
        contract::for_attr<implemented>::enforce,
        contract::for_attr<external>::out_of_scope,
        contract::for_attr<out_of_scope_policy>::out_of_scope>;
};

struct complete_log_adapter : log_adapter {
    using attribute_rules = contract::attribute_rules<
        contract::for_attr<documented>::display,
        contract::for_attr<enforced>::enforce,
        contract::for_attr<ignored>::ignore,
        contract::for_attr<unsupported>::display,
        contract::for_attr<implemented>::enforce,
        contract::for_attr<external>::out_of_scope,
        contract::for_attr<storage_only>::out_of_scope,
        contract::for_attr<out_of_scope_policy>::out_of_scope>;
};

struct Base {
    std::uint32_t id = 0;

    CONTRACT(Base,
        (id, 1, implemented{}, storage_only{}))
};

struct Middle : Base {
    CONTRACT(Middle, BASE(Base, 10))
};

struct Event : Middle {
    std::uint32_t value = 0;

    CONTRACT(Event,
        ATTRS(documented{}),
        BASE(Middle, 100),
        (value, 2,
            enforced{}, ignored{}, unsupported{}, hidden{}, external{}, out_of_scope_policy{}))
};

} // namespace attribute_validation_tests

int main() {
    using namespace attribute_validation_tests;
    using contract::attribute_validation_issue;
    using contract::attribute_validation_target;

    constexpr auto summary =
        contract::validate_adapter_mode<Event, log_adapter>();

    static_assert(summary.attribute_count == 9);
    static_assert(summary.visible_count == 8);
    static_assert(summary.guarantee_count == 2);
    static_assert(summary.ignored_count == 1);
    static_assert(summary.out_of_scope_count == 2);
    static_assert(summary.unsupported_count == 2);
    static_assert(summary.rejected_count == 0);
    static_assert(summary.error_count == 2);
    static_assert(!summary.valid());
    static_assert(!contract::adapter_mode_is_valid_v<Event, log_adapter>);

    constexpr auto complete_summary =
        contract::validate_adapter_mode<Event, complete_log_adapter>();
    static_assert(complete_summary.valid());
    static_assert(complete_summary.error_count == 0);
    static_assert(complete_summary.guarantee_count == 2);
    static_assert(contract::adapter_mode_is_valid_v<Event, complete_log_adapter>);
    contract::require_adapter_mode<Event, complete_log_adapter>();

    std::size_t entries = 0;
    bool saw_base_field = false;
    bool saw_out_of_scope = false;

    const auto runtime_summary =
        contract::validate_adapter_mode<Event, log_adapter>(
            [&](const auto& entry) {
                ++entries;

                using entry_type =
                    std::remove_cv_t<std::remove_reference_t<decltype(entry)>>;
                static_assert(entry_type::adapter == contract::adapter_type::log);

                if (entry.target == attribute_validation_target::field &&
                    entry.field_name == "id") {
                    saw_base_field = true;
                    assert(entry.field_id == 111);
                    assert(entry.declared_field_id == 1);
                    assert(entry.base_offset == 110);
                }

                if (entry.target == attribute_validation_target::field &&
                    entry.field_name == "value" &&
                    entry.resolution.mode == contract::attribute_mode::out_of_scope) {
                    saw_out_of_scope = true;
                }
            });

    assert(runtime_summary.attribute_count == entries);
    assert(saw_base_field);
    assert(saw_out_of_scope);

    return 0;
}
