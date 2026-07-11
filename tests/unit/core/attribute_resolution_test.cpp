// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>

namespace attribute_resolution_tests {

struct vocabulary {};
struct hidden_vocabulary {};

struct enforced {};
struct hinted {};
struct displayed {};
struct ignored {};
struct unsupported {};
struct hidden {};
struct render_policy {};
struct out_of_scope_policy {};
struct rejected_policy {};

struct adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::log;

    using visible_vocabularies = contract::vocabularies<vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::for_attr<enforced>::enforce,
        contract::for_attr<hinted>::hint,
        contract::for_attr<displayed>::display,
        contract::for_attr<ignored>::ignore,
        contract::for_attr<render_policy>::enforce,
        contract::for_attr<out_of_scope_policy>::out_of_scope,
        contract::for_attr<rejected_policy>::reject>;
};

struct storage_adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::storage;
    using visible_vocabularies = contract::vocabularies<vocabulary>;
};

struct audit_adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::debug;
    static constexpr contract::attribute_visibility visibility =
        contract::attribute_visibility::all_attrs_read_only;
};

} // namespace attribute_resolution_tests

namespace contract {

template<class Vocabulary>
struct resolution_test_attr_traits {
    using vocabulary = Vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<attribute_resolution_tests::enforced>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::hinted>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::displayed>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::ignored>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::unsupported>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::hidden>
    : resolution_test_attr_traits<attribute_resolution_tests::hidden_vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::render_policy>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::out_of_scope_policy>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

template<>
struct attr_traits<attribute_resolution_tests::rejected_policy>
    : resolution_test_attr_traits<attribute_resolution_tests::vocabulary> {};

} // namespace contract

int main() {
    using namespace attribute_resolution_tests;
    using contract::attribute_mode;
    using contract::attribute_resolution_kind;

    constexpr auto enforced_result =
        contract::resolve_attribute_mode<adapter>(enforced{});
    static_assert(enforced_result.kind == attribute_resolution_kind::visible);
    static_assert(enforced_result.mode == attribute_mode::enforce);

    static_assert(
        contract::resolve_attribute_mode<adapter>(hinted{}).mode ==
        attribute_mode::hint);
    static_assert(
        contract::resolve_attribute_mode<adapter>(displayed{}).mode ==
        attribute_mode::display);
    static_assert(
        contract::resolve_attribute_mode<adapter>(ignored{}).mode ==
        attribute_mode::ignore);
    static_assert(
        contract::resolve_attribute_mode<adapter>(unsupported{}).mode ==
        attribute_mode::error);

    constexpr auto hidden_result =
        contract::resolve_attribute_mode<adapter>(hidden{});
    static_assert(hidden_result.kind == attribute_resolution_kind::invisible);

    static_assert(
        contract::resolve_attribute_mode<audit_adapter>(unsupported{}).mode ==
        attribute_mode::display);
    static_assert(
        contract::resolve_attribute_mode<audit_adapter>(hidden{}).mode ==
        attribute_mode::display);

    constexpr auto implemented =
        contract::resolve_attribute_mode<adapter>(render_policy{});
    static_assert(implemented.kind == attribute_resolution_kind::visible);
    static_assert(implemented.mode == attribute_mode::enforce);

    static_assert(
        contract::resolve_attribute_mode<adapter>(out_of_scope_policy{}).mode ==
        attribute_mode::out_of_scope);
    static_assert(
        contract::resolve_attribute_mode<adapter>(rejected_policy{}).mode ==
        attribute_mode::reject);

    static_assert(
        contract::resolve_attribute_mode<storage_adapter>(render_policy{}).mode ==
        attribute_mode::error);

    return 0;
}
