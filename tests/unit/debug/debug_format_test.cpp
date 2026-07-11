// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base/format.hpp>
#include <contract/adapters/debug/format.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

int main() {
    assert(contract::adapters::base::escape_string("plain") == "plain");
    assert(contract::adapters::base::escape_string("\"quote\"") == "\\\"quote\\\"");
    assert(contract::adapters::base::escape_string("slash\\path") == "slash\\\\path");
    assert(contract::adapters::base::escape_string("line1\nline2") == "line1\\nline2");
    assert(contract::adapters::base::escape_string("tab\tvalue") == "tab\\tvalue");

    const char control[] = {'a', '\x01', 'b'};
    assert(contract::adapters::base::escape_string(std::string_view(control, sizeof(control))) == "a\\x01b");

    const auto short_text = contract::adapters::debug::truncate_string("abc", 10);
    assert(short_text.value == "abc");
    assert(!short_text.truncated());

    const auto long_text = contract::adapters::debug::truncate_string("abcdef", 3);
    assert(long_text.value == "abc");
    assert(long_text.omitted == 3);
    assert(long_text.truncated());

    assert(contract::adapters::base::quoted_string("a\nb") == "\"a\\nb\"");

    const std::array<std::uint8_t, 5> bytes{0x48, 0x65, 0x6c, 0x6c, 0x6f};
    assert(contract::adapters::debug::format_bytes_preview(bytes.data(), bytes.size(), 8) == "bytes[5] 48 65 6c 6c 6f");
    assert(contract::adapters::debug::format_bytes_preview(bytes.data(), bytes.size(), 3) == "bytes[5] 48 65 6c ...");

    const std::array<std::byte, 5> blob{
        std::byte{0x48},
        std::byte{0x65},
        std::byte{0x6c},
        std::byte{0x6c},
        std::byte{0x6f},
    };
    assert(contract::adapters::debug::format_bytes_preview_hex(blob.data(), blob.size(), 8) == "48 65 6c 6c 6f");
    assert(contract::adapters::debug::format_bytes_preview_hex(blob.data(), blob.size(), 3) == "48 65 6c ...");

    return 0;
}
