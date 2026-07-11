// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

struct DepthChild {
    std::string label = "leaf";

    CONTRACT(DepthChild, (label, 1))
};

struct DepthRecord {
    DepthChild child{};

    CONTRACT(DepthRecord, (child, 1))
};

struct ItemsRecord {
    std::vector<std::uint32_t> items{1, 2, 3, 4};

    CONTRACT(ItemsRecord, (items, 1))
};

} // namespace

int main() {
    {
        ItemsRecord record;

        contract::adapters::console::options opt;
        opt.max_items = 2;

        const std::string expected =
            "ItemsRecord:\n"
            "  items: # #1 std::vector<u32>, size=4\n"
            "    - 1 # [0]\n"
            "    - 2 # [1]\n"
            "    - ... # truncated, +2 items\n";

        assert(contract::adapters::console::to_string(record, opt) == expected);
    }

    {
        DepthRecord record;

        contract::adapters::console::options opt;
        opt.max_depth = 2;

        const std::string expected =
            "DepthRecord:\n"
            "  child: # #1 DepthChild\n"
            "    ... # max depth reached\n";

        assert(contract::adapters::console::to_string(record, opt) == expected);
    }

    return 0;
}
