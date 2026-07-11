#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <array>
#include <tuple>

namespace contract::adapters::protobuf {

namespace detail {

template<std::size_t Index>
struct tuple_element_field {
    static constexpr std::uint32_t id = static_cast<std::uint32_t>(Index + 1);
    static constexpr std::string_view name{};
    static constexpr bool is_base_import = false;
    static constexpr bool is_member_field = false;
    static constexpr bool is_reference_field = false;
    static constexpr bool is_property_field = false;
};

} // namespace detail

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    using tuple_type = std::tuple<Ts...>;

    template<class Writer>
    static write_status write(Writer& out, const tuple_type& value) {
        return write_elements<0>(out, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const tuple_type& value) {
        const auto size = measure_encoded_size(value);
        if (!size) {
            out.error().code(write_error_code::output_error)
                .field(field)
                .stage(detail::write_stage::length);
            return write_status::error;
        }

        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            out.error().field(field);
            return write_status::error;
        }
        if (out.write_varint(*size) == write_status::error) {
            out.error().field(field);
            return write_status::error;
        }
        const auto status = codec<tuple_type>::write(out, value);
        if (status == write_status::error) {
            out.error().field(field).stage(detail::write_stage::field_value);
        }
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, tuple_type& value) {
        std::array<bool, sizeof...(Ts)> seen{};
        std::size_t seen_count = 0;

        while (in.has_remaining()) {
            std::uint32_t field_number = 0;
            detail::wire_type wire{};
            if (in.read_tag(field_number, wire) == read_status::error) {
                return read_status::error;
            }

            if (field_number == 0) {
                in.error().code(read_error_code::field_number_zero)
                    .field_number(field_number)
                    .stage(detail::read_stage::field_key)
                    .wire(wire);
                return read_status::error;
            }

            if (field_number > sizeof...(Ts)) {
                in.error().code(read_error_code::unknown_field)
                    .field_number(field_number)
                    .stage(detail::read_stage::field_key)
                    .wire(wire);
                return read_status::error;
            }

            if (read_field<0>(in, value, seen, seen_count, field_number, wire) == read_status::error) {
                return read_status::error;
            }
        }

        if (seen_count != sizeof...(Ts)) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::field_value)
                .sizes(sizeof...(Ts), seen_count);
            return read_status::error;
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, tuple_type& value) {
        if (wire != detail::wire_type::length_delimited) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
            return read_status::error;
        }

        std::uint64_t length = 0;
        if (in.read_varint(length) == read_status::error) {
            in.error()
                .field(field)
                .wire(wire)
                .stage(detail::read_stage::length);
            return read_status::error;
        }

        const std::size_t body_size = static_cast<std::size_t>(length);
        const unsigned char* data = nullptr;
        if (in.read_view(body_size, data) == read_status::error) {
            in.error()
                .field(field)
                .wire(wire)
                .stage(detail::read_stage::field_value);
            return read_status::error;
        }

        reader<> nested{contract::io::window_input{data, body_size}, in.opt_};
        const auto status = codec<tuple_type>::read(nested, value);
        if (status == read_status::error) {
            in.error(nested.error(), body_size)
                .field(field)
                .wire(wire);
        }
        return status;
    }

private:
    template<std::size_t Index, class Writer>
    static write_status write_elements(Writer& out, const tuple_type& value) {
        if constexpr (Index >= sizeof...(Ts)) {
            return write_status::ok;
        } else {
            using element_type = std::tuple_element_t<Index, tuple_type>;
            const auto status = codec<element_type>::write(
                out,
                detail::tuple_element_field<Index>{},
                std::get<Index>(value));
            if (status == write_status::error) {
                return status;
            }
            return write_elements<Index + 1>(out, value);
        }
    }

    template<std::size_t Index, class Reader>
    static read_status read_field(
        Reader& in,
        tuple_type& value,
        std::array<bool, sizeof...(Ts)>& seen,
        std::size_t& seen_count,
        std::uint32_t field_number,
        detail::wire_type wire)
    {
        if constexpr (Index >= sizeof...(Ts)) {
            in.error().code(read_error_code::unknown_field)
                .field_number(field_number)
                .stage(detail::read_stage::field_key)
                .wire(wire);
            return read_status::error;
        } else {
            if (field_number == Index + 1) {
                using element_type = std::tuple_element_t<Index, tuple_type>;
                if (seen[Index]) {
                    in.error().code(read_error_code::duplicate_field)
                        .field_number(field_number)
                        .stage(detail::read_stage::field_key)
                        .wire(wire);
                    return read_status::error;
                }
                auto& slot = std::get<Index>(value);
                if (codec<element_type>::read(in, detail::tuple_element_field<Index>{}, wire, slot) == read_status::error) {
                    return read_status::error;
                }
                seen[Index] = true;
                ++seen_count;
                return read_status::ok;
            }
            return read_field<Index + 1>(in, value, seen, seen_count, field_number, wire);
        }
    }
};

} // namespace contract::adapters::protobuf
