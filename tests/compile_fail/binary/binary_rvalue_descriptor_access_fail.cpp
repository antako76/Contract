// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

struct SampleRecord {
    int value = 1;

    CONTRACT(SampleRecord, (value, 1))
};

struct SinkAdapter {
    template<class Field, class Value>
    void field(const Field&, const Value&) {}
};

int main() {
    auto field = contract::field_at<0, SampleRecord>();
    (void)field.get(SampleRecord{});

    SinkAdapter adapter;
    contract::visit(SampleRecord{}, adapter);
}
