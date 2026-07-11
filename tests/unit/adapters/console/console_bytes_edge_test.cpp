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

template<std::size_t N>
std::array<std::byte, N> make_bytes() {
    std::array<std::byte, N> data{};
    for (std::size_t i = 0; i < N; ++i) {
        data[i] = std::byte{static_cast<unsigned char>(0x10 + i)};
    }
    return data;
}

struct EmptyBlobRecord {
    std::array<std::byte, 0> permissions{};

    CONTRACT(EmptyBlobRecord, (permissions, 1))
};

struct ShortBlobRecord {
    std::array<std::byte, 3> permissions = make_bytes<3>();

    CONTRACT(ShortBlobRecord, (permissions, 1))
};

struct FullBlobRecord {
    std::array<std::byte, 4> permissions = make_bytes<4>();

    CONTRACT(FullBlobRecord, (permissions, 1))
};

struct TruncatedBlobRecord {
    std::array<std::byte, 5> permissions = make_bytes<5>();

    CONTRACT(TruncatedBlobRecord, (permissions, 1))
};

} // namespace

int main() {
    {
        EmptyBlobRecord record;
        const std::string expected =
            "EmptyBlobRecord:\n"
            "  permissions: \"\" # #1 std::array<std::byte,0>, bytes=0\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        ShortBlobRecord record;
        const std::string expected =
            "ShortBlobRecord:\n"
            "  permissions: \"10 11 12\" # #1 std::array<std::byte,3>, bytes=3\n";
        assert(contract::adapters::console::to_string(record) == expected);
    }

    {
        FullBlobRecord record;
        contract::adapters::console::options opt;
        opt.max_byte_preview_length = 4;

        const std::string expected =
            "FullBlobRecord:\n"
            "  permissions: \"10 11 12 13\" # #1 std::array<std::byte,4>, bytes=4\n";
        assert(contract::adapters::console::to_string(record, opt) == expected);
    }

    {
        TruncatedBlobRecord record;
        contract::adapters::console::options opt;
        opt.max_byte_preview_length = 3;

        const std::string expected =
            "TruncatedBlobRecord:\n"
            "  permissions: \"10 11 12 ...\" # #1 std::array<std::byte,5>, bytes=5, truncated\n";
        assert(contract::adapters::console::to_string(record, opt) == expected);
    }

    return 0;
}
