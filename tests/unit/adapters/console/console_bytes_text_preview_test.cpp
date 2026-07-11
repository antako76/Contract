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

// Byte-like fixed arrays are previewed as text when their content (after
// trimming a trailing zero run) is printable ASCII, regardless of the
// element type - this is a content decision, not a type decision.

struct PackedCharRecord {
    char code[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};

    CONTRACT(PackedCharRecord, (code, 1))
};

struct PaddedCharRecord {
    char code[16] = {'h', 'i'};

    CONTRACT(PaddedCharRecord, (code, 1))
};

struct AllZeroCharRecord {
    std::array<char, 4> code{};

    CONTRACT(AllZeroCharRecord, (code, 1))
};

struct EmbeddedNullRecord {
    // Trailing byte is zero (trimmed), but the trimmed content still has an
    // embedded NUL followed by more data - not text, must fall back to hex
    // of the FULL untrimmed buffer, not a truncated-looking string.
    char code[8] = {'a', 'b', 'c', '\0', 'd', 'e', 'f', '\0'};

    CONTRACT(EmbeddedNullRecord, (code, 1))
};

struct PrintableUnsignedCharRecord {
    // unsigned char with printable content also gets the text preview -
    // the decision is about content, not about char vs unsigned char.
    std::array<unsigned char, 5> label{'a', 'l', 'e', 'r', 't'};

    CONTRACT(PrintableUnsignedCharRecord, (label, 1))
};

struct BinaryUnsignedCharRecord {
    std::array<unsigned char, 4> hash{0x10, 0x2b, 0x9a, 0x01};

    CONTRACT(BinaryUnsignedCharRecord, (hash, 1))
};

struct LongPackedCharRecord {
    // 100 printable bytes, no trailing zero run - exceeds the default
    // max_string_length (64), so the text preview itself gets truncated.
    char code[100];

    LongPackedCharRecord() {
        for (char& c : code) {
            c = 'a';
        }
    }

    CONTRACT(LongPackedCharRecord, (code, 1))
};

} // namespace

int main() {
    {
        PackedCharRecord record;
        const std::string expected =
            "PackedCharRecord:\n"
            "  code: \"contract\" # #1 char[8], bytes=8\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        PaddedCharRecord record;
        const std::string expected =
            "PaddedCharRecord:\n"
            "  code: \"hi\" # #1 char[16], bytes=16\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        AllZeroCharRecord record;
        const std::string expected =
            "AllZeroCharRecord:\n"
            "  code: \"\" # #1 std::array<char,4>, bytes=4\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        EmbeddedNullRecord record;
        const std::string expected =
            "EmbeddedNullRecord:\n"
            "  code: \"61 62 63 00 64 65 66 00\" # #1 char[8], bytes=8\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        PrintableUnsignedCharRecord record;
        const std::string expected =
            "PrintableUnsignedCharRecord:\n"
            "  label: \"alert\" # #1 std::array<u8,5>, bytes=5\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        BinaryUnsignedCharRecord record;
        const std::string expected =
            "BinaryUnsignedCharRecord:\n"
            "  hash: \"10 2b 9a 01\" # #1 std::array<u8,4>, bytes=4\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        // Regression: the text preview must not emit its own inline
        // "truncated" comment on top of the array codec's count comment -
        // exactly one '#' comment segment, with one accurate truncated flag.
        LongPackedCharRecord record;
        const std::string output = contract::adapters::console::to_string(record);
        const std::string expected =
            "LongPackedCharRecord:\n"
            "  code: \"" + std::string(64, 'a') + "\" # #1 char[100], bytes=100, truncated\n";
        assert(output == expected);
        // Count comment *segments* (" # " delimiters), not raw '#' characters -
        // the field-id comment itself contains a '#' (e.g. "#1"), so a plain
        // character count would always be >= 2 for any commented field.
        std::size_t segment_count = 0;
        for (std::size_t pos = output.find(" # "); pos != std::string::npos;
             pos = output.find(" # ", pos + 1)) {
            ++segment_count;
        }
        assert(segment_count == 1);
    }

    return 0;
}
