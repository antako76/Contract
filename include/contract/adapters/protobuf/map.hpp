#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <map>

namespace contract::adapters::protobuf {

namespace detail {

struct map_entry_key_tag {};
struct map_entry_value_tag {};

struct map_entry_owner {
    map_entry_key_tag key;
    map_entry_value_tag value;

    CONTRACT(map_entry_owner,
        (key, 1),
        (value, 2)
    )
};

inline constexpr auto map_entry_fields = contract::flattened_fields_of<map_entry_owner>();
inline constexpr auto map_entry_key_field = std::get<0>(map_entry_fields);
inline constexpr auto map_entry_value_field = std::get<1>(map_entry_fields);

} // namespace detail

template<class K, class V, class Compare, class Allocator>
struct codec<std::map<K, V, Compare, Allocator>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::map<K, V, Compare, Allocator>& value) {
        for (const auto& item : value) {
            counting_output sizing{};
            writer<counting_output&> sizing_writer{sizing};

            using key_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::key_type>;
            if (codec<key_type>::write(sizing_writer, detail::map_entry_key_field, item.first) == write_status::error) {
                out.error()
                    .code(write_error_code::output_error)
                    .stage(detail::write_stage::length);
                return write_status::error;
            }

            using mapped_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::mapped_type>;
            if (codec<mapped_type>::write(sizing_writer, detail::map_entry_value_field, item.second) == write_status::error) {
                out.error()
                    .code(write_error_code::output_error)
                    .stage(detail::write_stage::length);
                return write_status::error;
            }

            const std::size_t entry_size = sizing_writer.position();
            if (out.write_varint(entry_size) == write_status::error) {
                return write_status::error;
            }

            if (codec<key_type>::write(out, detail::map_entry_key_field, item.first) == write_status::error) {
                out.error()
                    .stage(detail::write_stage::field_value);
                return write_status::error;
            }
            if (codec<mapped_type>::write(out, detail::map_entry_value_field, item.second) == write_status::error) {
                out.error()
                    .stage(detail::write_stage::field_value);
                return write_status::error;
            }
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::map<K, V, Compare, Allocator>& value) {
        for (const auto& item : value) {
            counting_output sizing{};
            writer<counting_output&> sizing_writer{sizing};

            using key_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::key_type>;
            if (codec<key_type>::write(sizing_writer, detail::map_entry_key_field, item.first) == write_status::error) {
                out.error()
                    .code(write_error_code::output_error)
                    .stage(detail::write_stage::length)
                    .field(field);
                return write_status::error;
            }

            using mapped_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::mapped_type>;
            if (codec<mapped_type>::write(sizing_writer, detail::map_entry_value_field, item.second) == write_status::error) {
                out.error()
                    .code(write_error_code::output_error)
                    .stage(detail::write_stage::length)
                    .field(field);
                return write_status::error;
            }

            const std::size_t entry_size = sizing_writer.position();
            if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
                out.error().field(field);
                return write_status::error;
            }
            if (out.write_varint(entry_size) == write_status::error) {
                out.error().field(field);
                return write_status::error;
            }

            if (codec<key_type>::write(out, detail::map_entry_key_field, item.first) == write_status::error) {
                out.error()
                    .field(field)
                    .stage(detail::write_stage::field_value);
                return write_status::error;
            }
            if (codec<mapped_type>::write(out, detail::map_entry_value_field, item.second) == write_status::error) {
                out.error()
                    .field(field)
                    .stage(detail::write_stage::field_value);
                return write_status::error;
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::map<K, V, Compare, Allocator>& value) {
        while (in.has_remaining()) {
            std::uint64_t length = 0;
            if (in.read_varint(length) == read_status::error) {
                in.error()
                    .wire(detail::wire_type::length_delimited)
                    .stage(detail::read_stage::length);
                return read_status::error;
            }

            const std::size_t entry_size = static_cast<std::size_t>(length);
            const unsigned char* data = nullptr;
            if (in.read_view(entry_size, data) == read_status::error) {
                in.error()
                    .wire(detail::wire_type::length_delimited)
                    .stage(detail::read_stage::field_value);
                return read_status::error;
            }

            reader<> nested{contract::io::window_input{data, entry_size}, in.opt_};
            using key_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::key_type>;
            using mapped_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::mapped_type>;
            key_type key{};
            mapped_type mapped{};

            while (nested.has_remaining()) {
                std::uint64_t key_wire = 0;
                if (nested.read_varint(key_wire) == read_status::error) {
                    in.error(nested.error(), entry_size)
                        .wire(detail::wire_type::length_delimited);
                    return read_status::error;
                }

                const std::uint32_t field_number = static_cast<std::uint32_t>(key_wire >> 3);
                const auto wire = static_cast<detail::wire_type>(key_wire & 0x7u);
                if (field_number == 1) {
                    if (codec<key_type>::read(nested, detail::map_entry_key_field, wire, key) == read_status::error) {
                        in.error(nested.error(), entry_size)
                            .wire(detail::wire_type::length_delimited);
                        return read_status::error;
                    }
                } else if (field_number == 2) {
                    if (codec<mapped_type>::read(nested, detail::map_entry_value_field, wire, mapped) == read_status::error) {
                        in.error(nested.error(), entry_size)
                            .wire(detail::wire_type::length_delimited);
                        return read_status::error;
                    }
                } else {
                    in.error(nested.error(), entry_size)
                        .code(field_number == 0 ? read_error_code::field_number_zero : read_error_code::unknown_field)
                        .field_number(field_number)
                        .wire(detail::wire_type::length_delimited);
                    return read_status::error;
                }
            }

            value.insert_or_assign(std::move(key), std::move(mapped));
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::map<K, V, Compare, Allocator>& value) {
        if (wire != detail::wire_type::length_delimited) {
            in.error()
                .code(read_error_code::wire_type_mismatch)
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

        const std::size_t entry_size = static_cast<std::size_t>(length);
        const unsigned char* data = nullptr;
        if (in.read_view(entry_size, data) == read_status::error) {
            in.error()
                .field(field)
                .wire(wire)
                .stage(detail::read_stage::field_value);
            return read_status::error;
        }

        reader<> nested{contract::io::window_input{data, entry_size}, in.opt_};
        using key_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::key_type>;
        using mapped_type = contract::adapters::base::clean_t<typename std::map<K, V, Compare, Allocator>::mapped_type>;
        key_type key{};
        mapped_type mapped{};

        while (nested.has_remaining()) {
            std::uint32_t field_number = 0;
            detail::wire_type wire_type{};
            if (nested.read_tag(field_number, wire_type) == read_status::error) {
                in.error(nested.error(), entry_size)
                    .field(field)
                    .wire(detail::wire_type::length_delimited);
                return read_status::error;
            }

            if (field_number == 1) {
                if (codec<key_type>::read(nested, detail::map_entry_key_field, wire_type, key) == read_status::error) {
                    in.error(nested.error(), entry_size)
                        .field(field)
                        .wire(detail::wire_type::length_delimited);
                    return read_status::error;
                }
            } else if (field_number == 2) {
                if (codec<mapped_type>::read(nested, detail::map_entry_value_field, wire_type, mapped) == read_status::error) {
                    in.error(nested.error(), entry_size)
                        .field(field)
                        .wire(detail::wire_type::length_delimited);
                    return read_status::error;
                }
            } else {
                in.error(nested.error(), entry_size)
                    .code(field_number == 0 ? read_error_code::field_number_zero : read_error_code::unknown_field)
                    .field(field)
                    .field_number(field_number)
                    .wire(detail::wire_type::length_delimited);
                return read_status::error;
            }
        }

        value.insert_or_assign(std::move(key), std::move(mapped));
        return read_status::ok;
    }
};

} // namespace contract::adapters::protobuf
