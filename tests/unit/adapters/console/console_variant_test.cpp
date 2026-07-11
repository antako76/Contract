// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>

namespace {

struct VariantChild {
    std::string name = "child";

    CONTRACT(VariantChild, (name, 1))
};

struct VariantLeafRecord {
    std::variant<std::uint32_t, std::string, VariantChild> payload{"queued"};

    CONTRACT(VariantLeafRecord, (payload, 1))
};

struct VariantBlockRecord {
    std::variant<std::uint32_t, std::string, VariantChild> payload{VariantChild{}};

    CONTRACT(VariantBlockRecord, (payload, 1))
};

} // namespace

int main() {
    VariantLeafRecord leaf;

    const std::string leaf_schema_expected =
        "VariantLeafRecord:\n"
        "  payload: # #1 std::variant<u32,std::string,VariantChild>, size=3\n"
        "    - \"queued\" # [1] std::string\n";

    assert(contract::adapters::console::to_string(leaf) == leaf_schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;

    const std::string leaf_value_expected =
        "VariantLeafRecord:\n"
        "  payload:\n"
        "    - \"queued\"\n";

    assert(contract::adapters::console::to_string(leaf, value_only) == leaf_value_expected);

    contract::adapters::console::options schema_no_indexes;
    schema_no_indexes.output_mode = contract::adapters::console::options::mode::schema;
    schema_no_indexes.show_indexes = false;

    const std::string leaf_no_indexes_expected =
        "VariantLeafRecord:\n"
        "  payload: # #1 std::variant<u32,std::string,VariantChild>, size=3\n"
        "    - \"queued\"\n";

    assert(contract::adapters::console::to_string(leaf, schema_no_indexes) == leaf_no_indexes_expected);

    VariantBlockRecord block;

    const std::string block_schema_expected =
        "VariantBlockRecord:\n"
        "  payload: # #1 std::variant<u32,std::string,VariantChild>, size=3\n"
        "    - # [2] VariantChild\n"
        "      VariantChild:\n"
        "        name: \"child\" # #1 std::string\n";

    assert(contract::adapters::console::to_string(block) == block_schema_expected);

    const std::string block_value_expected =
        "VariantBlockRecord:\n"
        "  payload:\n"
        "    -\n"
        "      VariantChild:\n"
        "        name: \"child\"\n";

    assert(contract::adapters::console::to_string(block, value_only) == block_value_expected);

    const std::string block_no_indexes_expected =
        "VariantBlockRecord:\n"
        "  payload: # #1 std::variant<u32,std::string,VariantChild>, size=3\n"
        "    -\n"
        "      VariantChild:\n"
        "        name: \"child\" # #1 std::string\n";

    assert(contract::adapters::console::to_string(block, schema_no_indexes) == block_no_indexes_expected);

    return 0;
}
