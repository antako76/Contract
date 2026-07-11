#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/tag.hpp>

#include <type_traits>
#include <utility>

namespace contract::detail {

template<class Object, auto MemberPtr>
concept can_member_ref = requires(Object&& object) {
    object.*MemberPtr;
};

template<class Field, class Object>
concept has_free_contract_get = requires(const Field& field, Object&& object) {
    contract_get(field, object);
};

template<class Field, class Object>
concept has_free_contract_get_tag = requires(contract::tag<Field> field, Object&& object) {
    contract_get(field, object);
};

template<class Field, class Object, class Value>
concept has_free_contract_set = requires(const Field& field, Object&& object, Value&& value) {
    contract_set(field, object, value);
};

template<class Field, class Object, class Value>
concept has_free_contract_set_tag = requires(contract::tag<Field> field, Object&& object, Value&& value) {
    contract_set(field, object, value);
};

template<class Object, class FieldTag>
concept has_member_contract_get_tag = requires(Object&& object) {
    object.contract_get(contract::tag<FieldTag>{});
};

template<class Object, class FieldTag, class Value>
concept has_member_contract_set_tag = requires(Object&& object, Value&& value) {
    object.contract_set(contract::tag<FieldTag>{}, value);
};

} // namespace contract::detail
