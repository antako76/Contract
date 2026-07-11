// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <string>
#include <string_view>

namespace {

struct AlignmentRecord {
    std::string short_text = "x";
    std::string long_text = "yz";

    CONTRACT(AlignmentRecord, (short_text, 1), (long_text, 2))
};

std::string_view line_after(std::string_view text, std::string_view needle) {
    const auto pos = text.find(needle);
    assert(pos != std::string_view::npos);

    const auto line_start = text.rfind('\n', pos);
    const auto start = line_start == std::string_view::npos ? 0 : line_start + 1;
    const auto line_end = text.find('\n', pos);
    return text.substr(start, line_end == std::string_view::npos ? text.size() - start : line_end - start);
}

} // namespace

int main() {
    AlignmentRecord record;

    contract::adapters::console::options schema;
    schema.max_string_length = 8;

    const auto actual = contract::adapters::console::to_string(record, schema);
    const auto short_line = line_after(actual, "short_text:");
    const auto long_line = line_after(actual, "long_text:");

    assert(short_line.find("# #1 std::string") != std::string_view::npos);
    assert(long_line.find("# #2 std::string") != std::string_view::npos);
    assert(short_line.find("# #1 std::string") == long_line.find("# #2 std::string"));

    return 0;
}
