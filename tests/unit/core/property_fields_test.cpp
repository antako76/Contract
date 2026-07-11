// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include "contract_test_types.hpp"

#include <contract/adapters/schema.hpp>

#include <cassert>
#include <cstdint>
#include <type_traits>

int main() {
    using namespace contract_tests;

    RequestStat stat;
    const auto duration = contract::field_at<0, RequestStat>();

    static_assert(std::is_same_v<decltype(duration)::storage_type, void>);
    static_assert(std::is_same_v<decltype(duration)::value_type, std::uint64_t>);

    assert(duration.get(stat) == 50);

    duration.set(stat, 75);

    assert(stat.finished_ns == 175);
    assert(debug_string(stat) == "duration_ns=75, started_ns=100, finished_ns=175");
    assert(contract::adapters::schema_string<RequestStat>() == "10 duration_ns\n11 started_ns\n12 finished_ns");

    return 0;
}
