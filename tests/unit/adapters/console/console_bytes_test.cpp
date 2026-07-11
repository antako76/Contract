// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/contract.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <string>

namespace {

struct BytesRecord {
    std::array<std::byte, 16> permissions {
        std::byte{0x10},
        std::byte{0x2b},
        std::byte{0x9a},
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04},
        std::byte{0x05},
        std::byte{0x06},
        std::byte{0x07},
        std::byte{0x08},
        std::byte{0x09},
        std::byte{0x0a},
        std::byte{0x0b},
        std::byte{0x0c},
        std::byte{0x0d},
    };

    CONTRACT(BytesRecord, (permissions, 1))
};

} // namespace

int main() {
    BytesRecord record;

    const std::string schema_expected =
        "BytesRecord:\n"
        "  permissions: \"10 2b 9a ...\" # #1 std::array<std::byte,16>, bytes=16, truncated\n";

    contract::adapters::console::options schema_short;
    schema_short.max_byte_preview_length = 3;
    assert(contract::adapters::console::to_string(record, schema_short) == schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;
    value_only.max_byte_preview_length = 3;

    const std::string value_expected =
        "BytesRecord:\n"
        "  permissions: \"10 2b 9a ...\"\n";

    assert(contract::adapters::console::to_string(record, value_only) == value_expected);

    return 0;
}
