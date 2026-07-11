// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>
#include <contract/check.hpp>
#include <contract/contract.hpp>
#include <contract/security.hpp>

#include <type_traits>

namespace adapter_traits_tests {

struct schema_vocabulary {};
struct security_vocabulary {};
struct storage_vocabulary {};

struct schema_name {};
struct secret {};
struct storage_hint {};

struct log_adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::log;

    using visible_vocabularies = contract::vocabularies<
        schema_vocabulary,
        security_vocabulary>;
};

struct audit_adapter {
    static constexpr contract::adapter_type type = contract::adapter_type::debug;
    static constexpr contract::attribute_visibility visibility =
        contract::attribute_visibility::all_attrs_read_only;
};

} // namespace adapter_traits_tests

namespace contract {

template<>
struct attr_traits<adapter_traits_tests::schema_name> {
    using vocabulary = adapter_traits_tests::schema_vocabulary;
    static constexpr attr_targets targets{true, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<adapter_traits_tests::secret> {
    using vocabulary = adapter_traits_tests::security_vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

template<>
struct attr_traits<adapter_traits_tests::storage_hint> {
    using vocabulary = adapter_traits_tests::storage_vocabulary;
    static constexpr attr_targets targets{false, true, false, false, false};
    static constexpr bool repeatable = false;
};

} // namespace contract

int main() {
    using namespace adapter_traits_tests;

    static_assert(contract::is_vocabulary_visible_v<log_adapter, schema_vocabulary>);
    static_assert(contract::is_attribute_visible_v<log_adapter, secret>);
    static_assert(!contract::is_attribute_visible_v<log_adapter, storage_hint>);
    static_assert(!contract::has_all_attributes_read_only_v<log_adapter>);

    static_assert(contract::has_all_attributes_read_only_v<audit_adapter>);
    static_assert(contract::is_attribute_visible_v<audit_adapter, schema_name>);
    static_assert(contract::is_attribute_visible_v<audit_adapter, storage_hint>);

    static_assert(contract::adapters::binary::adapter_traits::visibility ==
        contract::attribute_visibility::declared_vocabularies_only);
    static_assert(std::is_same_v<
        contract::adapters::binary::adapter_traits::visible_vocabularies,
        contract::vocabularies<
            contract::check::vocabulary,
            contract::security::vocabulary>>);
    static_assert(std::is_same_v<
        contract::adapters::binary::adapter_traits::attribute_rules,
        contract::attribute_rules<
            contract::default_for<contract::check::vocabulary>::ignore,
            contract::default_for<contract::security::vocabulary>::ignore,
            contract::for_tag<contract::check::tag::decode_guard>::enforce,
            contract::for_attr<contract::security::sensitive>::ignore,
            contract::for_attr<contract::security::secret>::ignore,
            contract::for_attr<contract::security::no_log>::out_of_scope,
            contract::for_attr<contract::security::encrypt>::enforce>>);

    return 0;
}
