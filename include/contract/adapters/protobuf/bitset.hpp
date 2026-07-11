#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <array>
#include <bitset>

namespace contract::adapters::protobuf {

template<std::size_t N>
struct codec<std::bitset<N>, void> {
    static constexpr std::size_t byte_count = (N + 7) / 8;

    template<class Writer>
    static write_status write(Writer& out, const std::bitset<N>& value) {
        if (out.write_varint(byte_count) == write_status::error) {
            return write_status::error;
        }

        if constexpr (byte_count == 0) {
            return write_status::ok;
        } else {
            std::array<unsigned char, byte_count> bytes{};
            for (std::size_t bit_index = 0; bit_index < N; ++bit_index) {
                if (value.test(bit_index)) {
                    bytes[bit_index / 8] |= static_cast<unsigned char>(1u << (bit_index % 8));
                }
            }
            return out.write(bytes.data(), byte_count);
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::bitset<N>& value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            out.error().field(field);
            return write_status::error;
        }
        const auto status = write(out, value);
        if (status == write_status::error) {
            out.error().field(field);
        }
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, std::bitset<N>& value) {
        value.reset();

        std::uint64_t size = 0;
        const auto length_status = in.read_varint(size);
        if (length_status == read_status::error) {
            in.error().stage(detail::read_stage::length);
            return length_status;
        }

        if (size != byte_count) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::length)
                .sizes(byte_count, static_cast<std::size_t>(size));
            return read_status::error;
        }

        if constexpr (byte_count == 0) {
            return read_status::ok;
        } else {
            const unsigned char* data = nullptr;
            const auto view_status = in.read_view(byte_count, data);
            if (view_status == read_status::error) {
                in.error().stage(detail::read_stage::field_value);
                return view_status;
            }

            for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
                const unsigned char byte = data[byte_index];
                for (std::size_t bit = 0; bit < 8; ++bit) {
                    const std::size_t bit_index = byte_index * 8 + bit;
                    if (bit_index < N && ((byte >> bit) & 0x1u)) {
                        value.set(bit_index);
                    }
                }
            }
            return read_status::ok;
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::bitset<N>& value) {
        if (wire != detail::wire_type::length_delimited) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
            return read_status::error;
        }

        const auto status = read(in, value);
        if (status == read_status::error) {
            in.error().field(field).wire(wire);
        }
        return status;
    }
};

} // namespace contract::adapters::protobuf
