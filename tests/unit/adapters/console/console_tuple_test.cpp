// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <string>
#include <tuple>

namespace {

struct TupleChild {
    std::string name = "child";

    CONTRACT(TupleChild, (name, 1))
};

struct TupleRecord {
    std::tuple<std::string, std::uint32_t, TupleChild> payload{"slot", 17, TupleChild{}};

    CONTRACT(TupleRecord, (payload, 1))
};

} // namespace

int main() {
    TupleRecord record;

    const std::string schema_expected =
        "TupleRecord:\n"
        "  payload: # #1 std::tuple<std::string,u32,TupleChild>, size=3\n"
        "    - \"slot\" # [0] std::string\n"
        "    - 17 # [1] u32\n"
        "    - # [2] TupleChild\n"
        "      TupleChild:\n"
        "        name: \"child\" # #1 std::string\n";

    assert(contract::adapters::console::to_string(record) == schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;

    const std::string value_expected =
        "TupleRecord:\n"
        "  payload:\n"
        "    - \"slot\"\n"
        "    - 17\n"
        "    -\n"
        "      TupleChild:\n"
        "        name: \"child\"\n";

    assert(contract::adapters::console::to_string(record, value_only) == value_expected);

    contract::adapters::console::options schema_no_indexes;
    schema_no_indexes.output_mode = contract::adapters::console::options::mode::schema;
    schema_no_indexes.show_indexes = false;

    const std::string no_indexes_expected =
        "TupleRecord:\n"
        "  payload: # #1 std::tuple<std::string,u32,TupleChild>, size=3\n"
        "    - \"slot\"\n"
        "    - 17\n"
        "    -\n"
        "      TupleChild:\n"
        "        name: \"child\" # #1 std::string\n";

    assert(contract::adapters::console::to_string(record, schema_no_indexes) == no_indexes_expected);

    return 0;
}
