// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <string_view>

namespace {

struct Numeric {
    std::int32_t a = 10;
    std::int32_t b = 20;
    std::int32_t c = 30;

    CONTRACT(Numeric, (a, 1), (b, 2), (c, 3))
};

struct RootBase {
    std::uint32_t root = 1;

    CONTRACT(RootBase, (root, 1))
};

struct MidBase : public RootBase {
    std::uint32_t mid = 3;

    CONTRACT(MidBase, BASE(RootBase, 100), (mid, 4))
};

struct LeafRecord : public MidBase {
    std::uint32_t leaf = 5;

    CONTRACT(LeafRecord, BASE(MidBase, 1000), (leaf, 7))
};

struct Empty {
    CONTRACT(Empty)
};

} // namespace

int main() {
    Numeric numeric;
    std::int32_t seen = -1;
    auto capture = [&](const auto& field) { seen = field.get(numeric); };

    assert(contract::dispatch_field_by_id<Numeric>(1, capture));
    assert(seen == 10);
    assert(contract::dispatch_field_by_id<Numeric>(2, capture));
    assert(seen == 20);
    assert(contract::dispatch_field_by_id<Numeric>(3, capture));
    assert(seen == 30);

    seen = -1;
    assert(!contract::dispatch_field_by_id<Numeric>(99, capture));
    assert(seen == -1); // fn must not fire when nothing matches

    // Effective ids after nested BASE flattening: root=1101, mid=1004, leaf=7.
    LeafRecord leaf_record;
    leaf_record.root = 11;
    leaf_record.mid = 33;
    leaf_record.leaf = 55;

    std::string_view seen_name;
    std::uint32_t seen_value = 0;
    auto capture_leaf = [&](const auto& field) {
        seen_name = field.name;
        seen_value = field.get(leaf_record);
    };

    assert(contract::dispatch_field_by_id<LeafRecord>(1101, capture_leaf));
    assert(seen_name == "root" && seen_value == 11);
    assert(contract::dispatch_field_by_id<LeafRecord>(1004, capture_leaf));
    assert(seen_name == "mid" && seen_value == 33);
    assert(contract::dispatch_field_by_id<LeafRecord>(7, capture_leaf));
    assert(seen_name == "leaf" && seen_value == 55);
    assert(!contract::dispatch_field_by_id<LeafRecord>(1, capture_leaf));

    bool never_called = false;
    auto never_call = [&](const auto&) { never_called = true; };
    assert(!contract::dispatch_field_by_id<Empty>(1, never_call));
    assert(!never_called);

    return 0;
}
