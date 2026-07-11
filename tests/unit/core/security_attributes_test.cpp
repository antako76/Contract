// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/attribute.hpp>
#include <contract/security.hpp>

#include <type_traits>

struct SecurityRecord {
    int value = 0;

    CONTRACT(SecurityRecord,
        (value, 1,
            contract::security::sensitive(),
            contract::security::secret(),
            contract::security::no_log(),
            contract::security::encrypt()))
};

struct SecurityAdapter {
    static constexpr contract::adapter_type type = contract::adapter_type::debug;

    using visible_vocabularies = contract::vocabularies<contract::security::vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::for_attr<contract::security::sensitive>::display,
        contract::for_attr<contract::security::secret>::enforce,
        contract::for_attr<contract::security::no_log>::enforce,
        contract::for_attr<contract::security::encrypt>::enforce>;
};

int main() {
    using field_type = std::remove_cv_t<std::remove_reference_t<decltype(contract::field_at<0, SecurityRecord>())>>;

    static_assert(contract::has_attribute_v<field_type, contract::security::sensitive>);
    static_assert(contract::has_attribute_v<field_type, contract::security::secret>);
    static_assert(contract::has_attribute_v<field_type, contract::security::no_log>);
    static_assert(contract::has_attribute_v<field_type, contract::security::encrypt>);

    static_assert(contract::is_attribute_visible_v<SecurityAdapter, contract::security::secret>);
    static_assert(contract::resolve_attribute_mode<SecurityAdapter>(contract::security::sensitive()).mode ==
        contract::attribute_mode::display);
    static_assert(contract::resolve_attribute_mode<SecurityAdapter>(contract::security::secret()).mode ==
        contract::attribute_mode::enforce);
    static_assert(contract::resolve_attribute_mode<SecurityAdapter>(contract::security::no_log()).mode ==
        contract::attribute_mode::enforce);
    static_assert(contract::resolve_attribute_mode<SecurityAdapter>(contract::security::encrypt()).mode ==
        contract::attribute_mode::enforce);

    return 0;
}
