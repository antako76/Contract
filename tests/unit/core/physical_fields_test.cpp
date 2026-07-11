// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include "contract_test_types.hpp"

#include <contract/adapters/schema.hpp>

#include <cassert>
#include <type_traits>

int main() {
    using namespace contract_tests;

    using Contract = decltype(contract::contract_of<BaseCounters>());
    static_assert(std::is_same_v<Contract::owner_type, BaseCounters>);
    static_assert(contract::field_count<BaseCounters>() == 4);
    static_assert(contract::type_name<BaseCounters>() == "BaseCounters");
    static_assert(contract::type_name<const BaseCounters&>() == "BaseCounters");

    using CountFieldRef = decltype(contract::field_at<0, BaseCounters>());
    using CountField = std::remove_cv_t<std::remove_reference_t<CountFieldRef>>;
    static_assert(!std::is_reference_v<CountFieldRef>);

    static_assert(std::is_same_v<CountField::owner_type, BaseCounters>);
    static_assert(std::is_same_v<CountField::storage_type, volatile unsigned long long>);
    static_assert(std::is_same_v<CountField::value_type, unsigned long long>);

    BaseCounters counters;
    const auto count_field = contract::field_at<0, BaseCounters>();

    static_assert(std::is_lvalue_reference_v<decltype(count_field.ref(counters))>);
    static_assert(std::is_volatile_v<std::remove_reference_t<decltype(count_field.ref(counters))>>);
    static_assert(std::is_same_v<decltype(count_field.get(counters)), unsigned long long>);

    count_field.set(counters, 42);
    assert(counters.count == 42);
    assert(count_field.get(counters) == 42);

    CountingAdapter adapter;
    contract::visit(counters, adapter);

    assert(adapter.fields == 4);
    assert(adapter.id_sum == 10);
    assert(debug_string(counters) == "count=42, error=2, time=30, clock=40");
    assert(contract::adapters::schema_string<BaseCounters>() == "1 count\n2 error\n3 time\n4 clock");

    return 0;
}
