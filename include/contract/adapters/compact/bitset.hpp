#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <array>
#include <bitset>

namespace contract::adapters::compact {

template<std::size_t N>
struct codec<std::bitset<N>, void> {
    static constexpr std::size_t byte_count = (N + 7u) / 8u;

    template<class Writer>
    static write_status write(Writer& out, const std::bitset<N>& value) {
        if (out.write_size_header(value_kind::bytes, byte_count) == write_status::error) {
            return out.error().stage(write_stage::size);
        }

        if constexpr (byte_count != 0) {
            std::array<unsigned char, byte_count> bytes{};
            for (std::size_t bit_index = 0; bit_index < N; ++bit_index) {
                if (value.test(bit_index)) {
                    bytes[bit_index / 8u] |= static_cast<unsigned char>(1u << (bit_index % 8u));
                }
            }
            if (out.write(bytes.data(), byte_count) == write_status::error) {
                return out.error().stage(write_stage::raw_bytes);
            }
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::bitset<N>& value) {
        if (out.write_size_header(value_kind::bytes, byte_count) == write_status::error) {
            return out.error().field(field).stage(write_stage::size);
        }

        if constexpr (byte_count != 0) {
            std::array<unsigned char, byte_count> bytes{};
            for (std::size_t bit_index = 0; bit_index < N; ++bit_index) {
                if (value.test(bit_index)) {
                    bytes[bit_index / 8u] |= static_cast<unsigned char>(1u << (bit_index % 8u));
                }
            }
            if (out.write(bytes.data(), byte_count) == write_status::error) {
                return out.error().field(field).stage(write_stage::raw_bytes);
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::bitset<N>& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::bytes, size) == read_status::error) {
            return in.error().stage(read_stage::size);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::size);
        }
        if (size != byte_count) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::size)
                .sizes(byte_count, static_cast<std::size_t>(size));
        }

        value.reset();
        if constexpr (byte_count != 0) {
            std::array<unsigned char, byte_count> bytes{};
            if (in.read(bytes.data(), byte_count) == read_status::error) {
                return in.error().stage(read_stage::raw_bytes);
            }
            for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
                for (std::size_t bit = 0; bit < 8u; ++bit) {
                    const std::size_t bit_index = byte_index * 8u + bit;
                    if (bit_index < N && ((bytes[byte_index] >> bit) & 0x1u)) {
                        value.set(bit_index);
                    }
                }
            }
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::bitset<N>& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::bytes, size) == read_status::error) {
            return in.error().field(field).stage(read_stage::size);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::size);
        }
        if (size != byte_count) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::size)
                .sizes(byte_count, static_cast<std::size_t>(size));
        }

        value.reset();
        if constexpr (byte_count != 0) {
            std::array<unsigned char, byte_count> bytes{};
            if (in.read(bytes.data(), byte_count) == read_status::error) {
                return in.error().field(field).stage(read_stage::raw_bytes);
            }
            for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
                for (std::size_t bit = 0; bit < 8u; ++bit) {
                    const std::size_t bit_index = byte_index * 8u + bit;
                    if (bit_index < N && ((bytes[byte_index] >> bit) & 0x1u)) {
                        value.set(bit_index);
                    }
                }
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::compact
