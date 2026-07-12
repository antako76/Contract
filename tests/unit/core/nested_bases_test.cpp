// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/schema.hpp>

#include <cassert>
#include <cstdint>
#include <string>

namespace {

struct RootBase {
    std::uint32_t root = 1;

    CONTRACT(RootBase,
        (root, 1),
        PROPERTY(root_twice, 10, std::uint32_t))

    std::uint32_t contract_get(const contract_fields::root_twice&) const {
        return root * 2;
    }

    void contract_set(const contract_fields::root_twice&, std::uint32_t value) {
        root = value / 2;
    }
};

struct MidBase : public RootBase {
    std::uint32_t mid = 3;

    CONTRACT(MidBase, BASE(RootBase, 100), (mid, 4))
};

struct LeafRecord : public MidBase {
    std::uint32_t leaf = 5;

    CONTRACT(LeafRecord, BASE(MidBase, 1000), (leaf, 7))
};

} // namespace

int main() {
    static_assert(contract::field_count<RootBase>() == 2);
    static_assert(contract::field_count<MidBase>() == 3);
    static_assert(contract::field_count<LeafRecord>() == 4);

    LeafRecord record;
    assert((contract::field_at<0, LeafRecord>().get(record) == 1));
    assert((contract::field_at<1, LeafRecord>().get(record) == 2));
    assert((contract::field_at<2, LeafRecord>().get(record) == 3));
    assert((contract::field_at<3, LeafRecord>().get(record) == 5));

    contract::field_at<1, LeafRecord>().set(record, 40);
    assert((record.root == 20));
    assert((contract::field_at<1, LeafRecord>().get(record) == 40));
    assert((contract::field_at<0, LeafRecord>().get(record) == 20));
    assert((contract::field_at<2, LeafRecord>().get(record) == 3));
    assert((contract::field_at<3, LeafRecord>().get(record) == 5));

    assert((contract::adapters::schema_string<RootBase>() == "1 root\n10 root_twice"));
    assert((contract::adapters::schema_string<MidBase>() == "101 root\n110 root_twice\n4 mid"));
    assert((contract::adapters::schema_string<LeafRecord>() == "1101 root\n1110 root_twice\n1004 mid\n7 leaf"));

    assert((contract::type_name<LeafRecord>() == "LeafRecord"));

    return 0;
}
