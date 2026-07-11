#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <bitset>

namespace contract::adapters::binary {

template<std::size_t N>
struct codec<std::bitset<N>, void> {
    static constexpr std::size_t byte_count = (N + 7) / 8;

    template<class Writer>
    static write_status write(Writer& out, const std::bitset<N>& value) {
        if constexpr (N <= 64) {
            if constexpr (N != 0) {
                const auto raw = static_cast<std::uint64_t>(value.to_ullong());
                return out.write(raw);
            }
        } else {
            for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
                unsigned char byte = 0;
                for (std::size_t bit = 0; bit < 8; ++bit) {
                    const std::size_t bit_index = byte_index * 8 + bit;
                    if (bit_index < N && value.test(bit_index)) {
                        byte |= static_cast<unsigned char>(1u << bit);
                    }
                }
                if (out.write(&byte, 1) == write_status::error) {
                    return write_status::error;
                }
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::bitset<N>& value) {
        value.reset();

        if constexpr (N <= 64) {
            if constexpr (N != 0) {
                std::uint64_t raw = 0;
                if (in.read(raw) == read_status::error) {
                    return read_status::error;
                }
                value = std::bitset<N>(raw);
            }
        } else {
            for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
                unsigned char byte = 0;
                if (in.read(&byte, 1) == read_status::error) {
                    return read_status::error;
                }
                for (std::size_t bit = 0; bit < 8; ++bit) {
                    const std::size_t bit_index = byte_index * 8 + bit;
                    if (bit_index < N && ((byte >> bit) & 0x1u)) {
                        value.set(bit_index);
                    }
                }
            }
        }
        return read_status::ok;
    }

};

} // namespace contract::adapters::binary
