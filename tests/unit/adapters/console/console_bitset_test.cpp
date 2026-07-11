// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <bitset>
#include <cstddef>
#include <string>

namespace {

std::bitset<256> make_permissions() {
    std::string bits(256, '0');
    bits.replace(0, 16, "1011001010110010");
    for (std::size_t i = 16; i < 45; ++i) {
        bits[i] = '1';
    }

    return std::bitset<256>(bits);
}

struct BitsetRecord {
    std::bitset<256> permissions = make_permissions();

    CONTRACT(BitsetRecord, (permissions, 1))
};

} // namespace

int main() {
    BitsetRecord record;

    const std::string schema_expected =
        "BitsetRecord:\n"
        "  permissions: \"1011001010110010...\" # #1 std::bitset<256>, set=37, truncated\n";

    contract::adapters::console::options schema_short;
    schema_short.max_string_length = 19;
    assert(contract::adapters::console::to_string(record, schema_short) == schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;
    value_only.max_string_length = 19;

    const std::string value_expected =
        "BitsetRecord:\n"
        "  permissions: \"1011001010110010...\"\n";

    assert(contract::adapters::console::to_string(record, value_only) == value_expected);

    contract::adapters::console::options schema_no_indexes = schema_short;
    schema_no_indexes.show_indexes = false;

    const std::string no_indexes_expected =
        "BitsetRecord:\n"
        "  permissions: \"1011001010110010...\" # #1 std::bitset<256>, set=37, truncated\n";

    assert(contract::adapters::console::to_string(record, schema_no_indexes) == no_indexes_expected);

    return 0;
}
