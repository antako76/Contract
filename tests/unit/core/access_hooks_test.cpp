// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include "contract_test_types.hpp"


#include <cassert>

int main() {
    using namespace contract_tests;

    HookedMetric hooked;
    const auto raw_count = contract::field_at<0, HookedMetric>();

    raw_count.set(hooked, 120);

    assert(hooked.raw_count == 12);
    assert(raw_count.get(hooked) == 120);
    assert(debug_string(hooked) == "raw_count=120");

    PlainMetric plain;
    const auto plain_raw_count = contract::field_at<0, PlainMetric>();

    plain_raw_count.set(plain, 77);

    assert(plain.raw_count == 77);
    assert(plain_raw_count.get(plain) == 77);

    FreeHookMetric free_hooked;
    const auto free_raw_count = contract::field_at<0, FreeHookMetric>();

    free_raw_count.set(free_hooked, 700);

    assert(free_hooked.raw_count == 7);
    assert(free_raw_count.get(free_hooked) == 700);
    assert(debug_string(free_hooked) == "raw_count=700");

    FreeTagMetric free_tagged;
    const auto free_tag_raw_count = contract::field_at<0, FreeTagMetric>();

    free_tag_raw_count.set(free_tagged, 5000);

    assert(free_tagged.raw_count == 5);
    assert(free_tag_raw_count.get(free_tagged) == 5000);

    RequestStat request_stat;
    const auto duration = contract::field_at<0, RequestStat>();

    assert(duration.get(request_stat) == 50);

    duration.set(request_stat, 30);

    assert(request_stat.finished_ns == 130);
    assert(duration.get(request_stat) == 30);

    FreePropertyStat free_property;
    const auto delta = contract::field_at<0, FreePropertyStat>();

    assert(delta.get(free_property) == 25);

    delta.set(free_property, 35);

    assert(free_property.high == 55);
    assert(debug_string(free_property) == "delta=35, low=20, high=55");

    FreeTagPropertyStat free_tag_property;
    const auto tag_delta = contract::field_at<0, FreeTagPropertyStat>();

    assert(tag_delta.get(free_tag_property) == 30);

    tag_delta.set(free_tag_property, 45);

    assert(free_tag_property.high == 75);
    assert(tag_delta.get(free_tag_property) == 45);

    ReferenceFallbackMetric reference_fallback;
    const auto reference_fallback_raw_count = contract::field_at<0, ReferenceFallbackMetric>();

    reference_fallback_raw_count.set(reference_fallback, 900);

    assert(reference_fallback.storage == 900);
    assert(reference_fallback_raw_count.get(reference_fallback) == 900);
    assert(reference_fallback_raw_count.kind == contract::field_kind::reference);

    ReferenceHookMetric reference_hooked;
    const auto reference_hook_raw_count = contract::field_at<0, ReferenceHookMetric>();

    reference_hook_raw_count.set(reference_hooked, 900);

    assert(reference_hooked.storage == 90);
    assert(reference_hook_raw_count.get(reference_hooked) == 900);
    assert(reference_hook_raw_count.kind == contract::field_kind::reference);

    ReferenceMetric reference_metric;
    const auto reference_raw_count = contract::field_at<0, ReferenceMetric>();

    reference_raw_count.set(reference_metric, 120);

    assert(reference_metric.storage == 12);
    assert(reference_raw_count.get(reference_metric) == 120);
    assert(reference_raw_count.kind == contract::field_kind::reference);

    return 0;
}
