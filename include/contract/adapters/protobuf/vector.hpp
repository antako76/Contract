#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <vector>

namespace contract::adapters::protobuf {

template<class T>
struct codec<std::vector<T>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (is_scalar_value_v<value_type>) {
            if (out.opt_.pack_repeated_scalars) {
                return write_packed_payload(out, value);
            }
        }
        return write_unpacked(out, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (is_scalar_value_v<value_type>) {
            if (out.opt_.pack_repeated_scalars) {
                if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
                    out.error().field(field);
                    return write_status::error;
                }
                const auto status = write_packed_payload(out, value);
                if (status == write_status::error) {
                    out.error().field(field);
                }
                return status;
            }
        }
        const auto status = write_unpacked(out, field, value);
        if (status == write_status::error) {
            out.error().field(field);
        }
        return status;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;

        if constexpr (is_scalar_value_v<value_type>) {
            if (wire == detail::wire_type::length_delimited) {
                std::uint64_t length = 0;
                if (in.read_varint(length) == read_status::error) {
                    in.error().field(field).wire(wire).stage(detail::read_stage::length);
                    return read_status::error;
                }

                const unsigned char* data = nullptr;
                if (in.read_view(static_cast<std::size_t>(length), data) == read_status::error) {
                    in.error().field(field).wire(wire).stage(detail::read_stage::length);
                    return read_status::error;
                }

        reader<contract::io::window_input> nested{
            contract::io::window_input{data, static_cast<std::size_t>(length)},
            in.opt_};
                std::size_t element_index = 0;
                while (nested.has_remaining()) {
                    value_type item{};
                    if (codec<value_type>::read(nested, item) == read_status::error) {
                        in.error(nested.error(), static_cast<std::size_t>(length))
                            .field(field)
                            .wire(wire)
                            .element_index(element_index, element_index + 1);
                        return read_status::error;
                    }
                    value.push_back(std::move(item));
                    ++element_index;
                }
                return read_status::ok;
            }
        }

        const std::size_t element_index = value.size();
        value_type item{};
        if (codec<value_type>::read(in, field, wire, item) == read_status::error) {
            in.error().field(field).wire(wire).element_index(element_index, value.size() + 1);
            return read_status::error;
        }
        value.push_back(std::move(item));
        return read_status::ok;
    }

private:
    template<class U>
    static constexpr bool is_scalar_value_v =
        std::is_same_v<U, bool> ||
        (std::is_integral_v<U> &&
         !std::is_same_v<U, char> &&
         !std::is_same_v<U, signed char> &&
         !std::is_same_v<U, unsigned char>) ||
        std::is_enum_v<U> ||
        std::is_same_v<U, float> ||
        std::is_same_v<U, double>;

    template<class Writer>
    static write_status write_packed_payload(Writer& out, const std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (is_scalar_value_v<value_type>) {
            if (out.opt_.pack_repeated_scalars) {
                if (value.empty()) {
                    return write_status::ok;
                }

                counting_output sizing{};
                writer<counting_output&> sizing_writer{sizing};
                const std::size_t size = value.size();
                for (std::size_t element_index = 0; element_index < size; ++element_index) {
                    if (codec<value_type>::write(sizing_writer, value[element_index]) == write_status::error) {
                        out.error().code(write_error_code::output_error)
                            .stage(detail::write_stage::length)
                            .element_index(element_index, size);
                        return write_status::error;
                    }
                }
                const std::size_t payload_size = sizing_writer.position();

                if (out.write_varint(payload_size) == write_status::error) {
                    return write_status::error;
                }
                for (std::size_t element_index = 0; element_index < size; ++element_index) {
                    if (codec<value_type>::write(out, value[element_index]) == write_status::error) {
                        out.error().element_index(element_index, size);
                        return write_status::error;
                    }
                }
                return write_status::ok;
            }
        }

        const std::size_t size = value.size();
        for (std::size_t element_index = 0; element_index < size; ++element_index) {
            if (codec<value_type>::write(out, value[element_index]) == write_status::error) {
                out.error().element_index(element_index, size);
                return write_status::error;
            }
        }
        return write_status::ok;
    }

    template<class Writer>
    static write_status write_unpacked(Writer& out, const std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        for (std::size_t element_index = 0; element_index < value.size(); ++element_index) {
            if (codec<value_type>::write(out, value[element_index]) == write_status::error) {
                out.error().element_index(element_index);
                return write_status::error;
            }
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write_unpacked(Writer& out, const Field& field, const std::vector<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        const std::size_t size = value.size();
        for (std::size_t element_index = 0; element_index < size; ++element_index) {
            if (codec<value_type>::write(out, field, value[element_index]) == write_status::error) {
                out.error().element_index(element_index, size);
                return write_status::error;
            }
        }
        return write_status::ok;
    }
};

} // namespace contract::adapters::protobuf
