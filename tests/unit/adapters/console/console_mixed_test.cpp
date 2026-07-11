// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include "contract_test_types.hpp"

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

struct ConsoleInner {
    std::string name = "inner";
    std::uint32_t count = 7;

    CONTRACT(ConsoleInner, (name, 1), (count, 2))
};

struct ConsoleMixedRecord {
    std::string title = "integration";
    std::vector<std::string> tags{"payment", "critical"};
    std::tuple<std::string, std::uint32_t> stage{"payment", 2};
    std::variant<std::string, std::uint32_t, ConsoleInner> status{ConsoleInner{}};
    std::optional<ConsoleInner> maybe_inner{ConsoleInner{}};
    std::map<std::string, std::string> headers{
        {"content-type", "application/json"},
        {"x-request-id", "abc"},
    };
    std::unordered_map<std::string, std::string> labels{
        {"priority", "high"},
    };
    std::bitset<10> flags{0b1010010110};
    std::array<std::byte, 4> permissions{
        std::byte{0x10},
        std::byte{0x2b},
        std::byte{0x9a},
        std::byte{0x03},
    };

    CONTRACT(ConsoleMixedRecord,
        (title, 1),
        (tags, 2),
        (stage, 3),
        (status, 4),
        (maybe_inner, 5),
        (headers, 6),
        (labels, 7),
        (flags, 8),
        (permissions, 9))
};

} // namespace

int main() {
    ConsoleMixedRecord record;

    contract::adapters::console::options schema;
    schema.align_comments = true;

    const std::string expected =
        "ConsoleMixedRecord:\n"
        "  title: \"integration\"                  # #1 std::string\n"
        "  tags:                                 # #2 std::vector<std::string>, size=2\n"
        "    - \"payment\"                         # [0]\n"
        "    - \"critical\"                        # [1]\n"
        "  stage:                                # #3 std::tuple<std::string,u32>, size=2\n"
        "    - \"payment\"                         # [0] std::string\n"
        "    - 2                                 # [1] u32\n"
        "  status:                               # #4 std::variant<std::string,u32,ConsoleInner>, size=3\n"
        "    -                                   # [2] ConsoleInner\n"
        "      ConsoleInner:\n"
        "        name: \"inner\"                   # #1 std::string\n"
        "        count: 7                        # #2 u32\n"
        "  maybe_inner:                          # #5 std::optional<ConsoleInner>\n"
        "    ConsoleInner:\n"
        "      name: \"inner\"                     # #1 std::string\n"
        "      count: 7                          # #2 u32\n"
        "  headers:                              # #6 std::map<std::string,std::string>, size=2\n"
        "    \"content-type\": \"application/json\"  # [0]\n"
        "    \"x-request-id\": \"abc\"               # [1]\n"
        "  labels:                               # #7 std::unordered_map<std::string,std::string>, size=1\n"
        "    \"priority\": \"high\"                  # [0]\n"
        "  flags: \"1010010110\"                   # #8 std::bitset<10>, set=5\n"
        "  permissions: \"10 2b 9a 03\"            # #9 std::array<std::byte,4>, bytes=4\n";

    assert(contract::adapters::console::to_string(record, schema) == expected);

    return 0;
}
