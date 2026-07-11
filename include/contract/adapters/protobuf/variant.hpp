#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <variant>

namespace contract::adapters::protobuf {

template<class... Ts>
struct codec<std::variant<Ts...>, void> {
    using variant_type = std::variant<Ts...>;
    static_assert(sizeof...(Ts) > 0, "protobuf variant codec requires at least one alternative");

    template<class Writer>
    static write_status write(Writer& out, const variant_type& value) {
        if (value.valueless_by_exception()) {
            out.error().code(write_error_code::output_error)
                .stage(detail::write_stage::field_value);
            return write_status::error;
        }

        return std::visit(
            [&](const auto& item) -> write_status {
                using alt_type = contract::adapters::base::clean_t<decltype(item)>;
                const auto payload_size = measure_encoded_size(item);
                if (!payload_size) {
                    out.error().code(write_error_code::output_error)
                        .stage(detail::write_stage::length);
                    return write_status::error;
                }

                if (out.write_tag(1u, detail::wire_type::varint) == write_status::error) {
                    return write_status::error;
                }
                if (out.write_varint(static_cast<std::uint64_t>(value.index())) == write_status::error) {
                    return write_status::error;
                }
                if (out.write_tag(2u, detail::wire_type::length_delimited) == write_status::error) {
                    return write_status::error;
                }
                if (out.write_varint(*payload_size) == write_status::error) {
                    return write_status::error;
                }

                const auto status = codec<alt_type>::write(out, item);
                if (status == write_status::error) {
                    out.error().stage(detail::write_stage::field_value);
                }
                return status;
            },
            value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const variant_type& value) {
        const auto size = measure_encoded_size(value);
        if (!size) {
            out.error()
                .code(write_error_code::output_error)
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
        const auto status = codec<variant_type>::write(out, value);
        if (status == write_status::error) {
            out.error().field(field).stage(detail::write_stage::field_value);
        }
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, variant_type& value) {
        std::uint32_t field_number = 0;
        detail::wire_type wire{};
        if (in.read_tag(field_number, wire) == read_status::error) {
            return read_status::error;
        }
        if (field_number != 1) {
            in.error()
                .code(field_number == 0 ? read_error_code::field_number_zero : read_error_code::unknown_field)
                .field_number(field_number)
                .stage(detail::read_stage::field_key)
                .wire(wire);
            return read_status::error;
        }
        if (wire != detail::wire_type::varint) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field_number(field_number)
                .wire(wire)
                .expected_wire(detail::wire_type::varint);
            return read_status::error;
        }

        std::uint64_t raw_index = 0;
        if (in.read_varint(raw_index) == read_status::error) {
            return read_status::error;
        }

        if (in.read_tag(field_number, wire) == read_status::error) {
            return read_status::error;
        }
        if (field_number != 2) {
            in.error()
                .code(field_number == 0 ? read_error_code::field_number_zero : read_error_code::unknown_field)
                .field_number(field_number)
                .stage(detail::read_stage::field_key)
                .wire(wire);
            return read_status::error;
        }
        if (wire != detail::wire_type::length_delimited) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field_number(field_number)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
            return read_status::error;
        }

        std::uint64_t payload_size = 0;
        if (in.read_varint(payload_size) == read_status::error) {
            return read_status::error;
        }

        const unsigned char* data = nullptr;
        if (in.read_view(static_cast<std::size_t>(payload_size), data) == read_status::error) {
            return read_status::error;
        }

        reader<> nested{contract::io::window_input{data, static_cast<std::size_t>(payload_size)}, in.opt_};
        const auto status = read_alternative<0>(nested, static_cast<std::size_t>(raw_index), value);
        if (status == read_status::error) {
            return status;
        }

        if (nested.has_remaining()) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::field_value)
                    .sizes(static_cast<std::size_t>(payload_size), nested.position());
            return read_status::error;
        }

        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, variant_type& value) {
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
        const auto status = codec<variant_type>::read(nested, value);
        if (status == read_status::error) {
            in.error(nested.error(), body_size)
                .field(field)
                .wire(wire);
        }
        return status;
    }

private:
    template<std::size_t Index, class Reader>
    static read_status read_alternative(
        Reader& in,
        std::size_t index,
        variant_type& value)
    {
        if constexpr (Index >= sizeof...(Ts)) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::field_value)
                .sizes(sizeof...(Ts), index);
            return read_status::error;
        } else if (index != Index) {
            return read_alternative<Index + 1>(in, index, value);
        } else {
            using alt_type = std::variant_alternative_t<Index, variant_type>;
            auto& slot = value.template emplace<Index>();
            if constexpr (contract::adapters::base::has_contract_definition<alt_type>) {
                if (in.read_message(slot) == read_status::error) {
                    return read_status::error;
                }
            } else {
                if (codec<alt_type>::read(in, slot) == read_status::error) {
                    return read_status::error;
                }
            }
            return read_status::ok;
        }
    }
};

} // namespace contract::adapters::protobuf
