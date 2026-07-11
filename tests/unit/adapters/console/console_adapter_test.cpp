// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace {

struct StringOutput {
    std::string text;

    StringOutput& operator<<(std::string_view value) {
        text += value;
        return *this;
    }

    StringOutput& operator<<(const std::string& value) {
        text += value;
        return *this;
    }

    StringOutput& operator<<(char value) {
        text.push_back(value);
        return *this;
    }

    template<class T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
    StringOutput& operator<<(T value) {
        text += std::to_string(value);
        return *this;
    }
};

struct ConsoleChild {
    std::string label = "child\nvalue";

    CONTRACT(ConsoleChild, (label, 1))
};

struct ConsoleRecord {
    std::uint32_t id = 42;
    std::string route = "/api/payments/{id}";
    bool enabled = true;
    ConsoleChild child;
    std::vector<std::string> tags{"payment", "critical"};
    std::array<std::uint32_t, 3> codes{{7, 8, 9}};
    std::uint32_t raw_codes[3]{1, 2, 3};
    std::optional<std::uint64_t> user_id{777};
    std::optional<ConsoleChild> maybe_child{ConsoleChild{}};

    CONTRACT(ConsoleRecord,
        (id, 1),
        (route, 2),
        (enabled, 3),
        (child, 4),
        (tags, 5),
        (codes, 6),
        (raw_codes, 7),
        (user_id, 8),
        (maybe_child, 9))
};

} // namespace

int main() {
    ConsoleRecord record;

    const auto actual = contract::adapters::console::to_string(record);
    const std::string expected =
        "ConsoleRecord:\n"
        "  id: 42 # #1 u32\n"
        "  route: \"/api/payments/{id}\" # #2 std::string\n"
        "  enabled: true # #3 bool\n"
        "  child: # #4 ConsoleChild\n"
        "    ConsoleChild:\n"
        "      label: \"child\\nvalue\" # #1 std::string\n"
        "  tags: # #5 std::vector<std::string>, size=2\n"
        "    - \"payment\" # [0]\n"
        "    - \"critical\" # [1]\n"
        "  codes: # #6 std::array<u32,3>, size=3\n"
        "    - 7 # [0]\n"
        "    - 8 # [1]\n"
        "    - 9 # [2]\n"
        "  raw_codes: # #7 u32[3], size=3\n"
        "    - 1 # [0]\n"
        "    - 2 # [1]\n"
        "    - 3 # [2]\n"
        "  user_id: 777 # #8 std::optional<u64>\n"
        "  maybe_child: # #9 std::optional<ConsoleChild>\n"
        "    ConsoleChild:\n"
        "      label: \"child\\nvalue\" # #1 std::string\n";

    assert(actual == expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;

    const auto value_actual = contract::adapters::console::to_string(record, value_only);
    const std::string value_expected =
        "ConsoleRecord:\n"
        "  id: 42\n"
        "  route: \"/api/payments/{id}\"\n"
        "  enabled: true\n"
        "  child:\n"
        "    ConsoleChild:\n"
        "      label: \"child\\nvalue\"\n"
        "  tags:\n"
        "    - \"payment\"\n"
        "    - \"critical\"\n"
        "  codes:\n"
        "    - 7\n"
        "    - 8\n"
        "    - 9\n"
        "  raw_codes:\n"
        "    - 1\n"
        "    - 2\n"
        "    - 3\n"
        "  user_id: 777\n"
        "  maybe_child:\n"
        "    ConsoleChild:\n"
        "      label: \"child\\nvalue\"\n";

    assert(value_actual == value_expected);

    contract::adapters::console::options colored = value_only;
    colored.color.enabled = true;

    const auto colored_actual = contract::adapters::console::to_string(record, colored);
    assert(colored_actual.find("\x1b[96mConsoleRecord\x1b[0m") != std::string::npos);
    assert(colored_actual.find("\x1b[92m\"payment\"\x1b[0m") != std::string::npos);
    assert(colored_actual.find("\x1b[33m42\x1b[0m") != std::string::npos);

    StringOutput output;
    contract::adapters::console::writer<StringOutput&> custom_out(output, value_only);
    custom_out << record;
    assert(output.text == value_expected);

    return 0;
}
