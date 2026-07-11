#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <array>
#include <cstring>

namespace contract::adapters::protobuf {

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::array<T, N>& value) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return write_bytes(out, value);
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            return write_packed(out, value);
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::array<T, N>& value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            out.error().field(field);
            return write_status::error;
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            const auto status = write_bytes(out, value);
            if (status == write_status::error) {
                out.error().field(field);
            }
            return status;
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            const auto status = write_packed(out, value);
            if (status == write_status::error) {
                out.error().field(field);
            }
            return status;
        }
    }

    template<class Reader>
    static read_status read(Reader& in, std::array<T, N>& value) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, value);
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            return read_packed(in, value);
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::array<T, N>& value) {
        if (wire != detail::wire_type::length_delimited) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
            return read_status::error;
        }

        if constexpr (detail::is_byte_like_element_v<T>) {
            const auto status = read_bytes(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
            }
            return status;
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            const auto status = read_packed(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
                return status;
            }
            return read_status::ok;
        }
    }

private:
    template<class Writer>
    static write_status write_bytes(Writer& out, const std::array<T, N>& value) {
        const std::size_t trimmed = detail::trim_trailing_zeros(value.data(), N);
        if (out.write_varint(trimmed) == write_status::error) {
            return write_status::error;
        }
        if (trimmed != 0 && out.write(value.data(), trimmed * sizeof(T)) == write_status::error) {
            return write_status::error;
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read_bytes(Reader& in, std::array<T, N>& value) {
        std::uint64_t length = 0;
        if (in.read_varint(length) == read_status::error) {
            return in.error().stage(detail::read_stage::length);
        }
        if (length > N * sizeof(T)) {
            return in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::length)
                .sizes(N * sizeof(T), static_cast<std::size_t>(length));
        }
        const unsigned char* data = nullptr;
        if (in.read_view(static_cast<std::size_t>(length), data) == read_status::error) {
            return in.error().stage(detail::read_stage::field_value);
        }
        if (length != 0) {
            std::memcpy(value.data(), data, static_cast<std::size_t>(length));
        }
        if (length != N * sizeof(T)) {
            std::memset(value.data() + static_cast<std::size_t>(length), 0, N * sizeof(T) - static_cast<std::size_t>(length));
        }
        return read_status::ok;
    }
    template<class Writer>
    static write_status write_packed(Writer& out, const std::array<T, N>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        static_assert(is_scalar_value_v<value_type>,
            "protobuf array codec supports only scalar-like value types");

        if constexpr (N == 0) {
            return out.write_varint(0);
        } else {
            counting_output sizing{};
            writer<counting_output&> sizing_writer{sizing};
            for (std::size_t element_index = 0; element_index < N; ++element_index) {
                if (codec<value_type>::write(sizing_writer, value[element_index]) == write_status::error) {
                    out.error().code(write_error_code::output_error)
                        .stage(detail::write_stage::length)
                        .element_index(element_index, N);
                    return write_status::error;
                }
            }

            const std::size_t payload_size = sizing_writer.position();
            if (out.write_varint(payload_size) == write_status::error) {
                return write_status::error;
            }

            for (std::size_t element_index = 0; element_index < N; ++element_index) {
                if (codec<value_type>::write(out, value[element_index]) == write_status::error) {
                    out.error().element_index(element_index, N);
                    return write_status::error;
                }
            }
            return write_status::ok;
        }
    }

    template<class Reader>
    static read_status read_packed(Reader& in, std::array<T, N>& value)
    {
        using value_type = contract::adapters::base::clean_t<T>;
        std::uint64_t length = 0;
        const auto length_status = in.read_varint(length);
        if (length_status == read_status::error) {
            in.error().stage(detail::read_stage::length);
            return length_status;
        }

        const unsigned char* data = nullptr;
        const auto view_status = in.read_view(static_cast<std::size_t>(length), data);
        if (view_status == read_status::error) {
            in.error().stage(detail::read_stage::length);
            return view_status;
        }

        reader<contract::io::window_input> nested{
            contract::io::window_input{data, static_cast<std::size_t>(length)},
            in.opt_};

        std::size_t element_index = 0;
        while (nested.has_remaining()) {
            if (element_index >= N) {
                in.error().code(read_error_code::invalid_size)
                    .stage(detail::read_stage::field_value)
                    .element_index(element_index, N)
                    .sizes(N, element_index + 1);
                return read_status::error;
            }

            if (codec<value_type>::read(nested, value[element_index]) == read_status::error) {
                in.error(nested.error(), static_cast<std::size_t>(length))
                    .element_index(element_index, N);
                return read_status::error;
            }
            ++element_index;
        }

        if (element_index != N) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::field_value)
                .element_index(element_index, N)
                .sizes(N, element_index);
            return read_status::error;
        }

        return read_status::ok;
    }

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
};

template<class T, std::size_t Size>
struct codec<T[Size], void> {
    template<class Writer>
    static write_status write(Writer& out, const T (&value)[Size]) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return write_bytes(out, value);
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            return write_packed(out, value);
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T (&value)[Size]) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            out.error().field(field);
            return write_status::error;
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            const auto status = write_bytes(out, value);
            if (status == write_status::error) {
                out.error().field(field);
            }
            return status;
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            const auto status = write_packed(out, value);
            if (status == write_status::error) {
                out.error().field(field);
            }
            return status;
        }
    }

    template<class Reader>
    static read_status read(Reader& in, T (&value)[Size]) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, value);
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            return read_packed(in, value);
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, T (&value)[Size]) {
        if (wire != detail::wire_type::length_delimited) {
            in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
            return read_status::error;
        }

        if constexpr (detail::is_byte_like_element_v<T>) {
            const auto status = read_bytes(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
            }
            return status;
        } else {
            using value_type = contract::adapters::base::clean_t<T>;
            static_assert(is_scalar_value_v<value_type>,
                "protobuf array codec supports only scalar-like value types");

            const auto status = read_packed(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
                return status;
            }
            return read_status::ok;
        }
    }

private:
    template<class Writer>
    static write_status write_bytes(Writer& out, const T (&value)[Size]) {
        const std::size_t trimmed = detail::trim_trailing_zeros(value, Size);
        if (out.write_varint(trimmed) == write_status::error) {
            return write_status::error;
        }
        if (trimmed != 0 && out.write(value, trimmed * sizeof(T)) == write_status::error) {
            return write_status::error;
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read_bytes(Reader& in, T (&value)[Size]) {
        std::uint64_t length = 0;
        if (in.read_varint(length) == read_status::error) {
            return in.error().stage(detail::read_stage::length);
        }
        if (length > Size * sizeof(T)) {
            return in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::length)
                .sizes(Size * sizeof(T), static_cast<std::size_t>(length));
        }
        const unsigned char* data = nullptr;
        if (in.read_view(static_cast<std::size_t>(length), data) == read_status::error) {
            return in.error().stage(detail::read_stage::field_value);
        }
        if (length != 0) {
            std::memcpy(value, data, static_cast<std::size_t>(length));
        }
        if (length != Size * sizeof(T)) {
            std::memset(reinterpret_cast<unsigned char*>(value) + static_cast<std::size_t>(length), 0,
                Size * sizeof(T) - static_cast<std::size_t>(length));
        }
        return read_status::ok;
    }

    template<class Writer>
    static write_status write_packed(Writer& out, const T (&value)[Size]) {
        using value_type = contract::adapters::base::clean_t<T>;
        static_assert(is_scalar_value_v<value_type>,
            "protobuf array codec supports only scalar-like value types");

        if constexpr (Size == 0) {
            return out.write_varint(0);
        } else {
            counting_output sizing{};
            writer<counting_output&> sizing_writer{sizing};
            for (std::size_t element_index = 0; element_index < Size; ++element_index) {
                if (codec<value_type>::write(sizing_writer, value[element_index]) == write_status::error) {
                    out.error().code(write_error_code::output_error)
                        .stage(detail::write_stage::length)
                        .element_index(element_index, Size);
                    return write_status::error;
                }
            }

            const std::size_t payload_size = sizing_writer.position();
            if (out.write_varint(payload_size) == write_status::error) {
                return write_status::error;
            }

            for (std::size_t element_index = 0; element_index < Size; ++element_index) {
                if (codec<value_type>::write(out, value[element_index]) == write_status::error) {
                    out.error().element_index(element_index, Size);
                    return write_status::error;
                }
            }
            return write_status::ok;
        }
    }

    template<class Reader>
    static read_status read_packed(Reader& in, T (&value)[Size])
    {
        using value_type = contract::adapters::base::clean_t<T>;
        std::uint64_t length = 0;
        const auto length_status = in.read_varint(length);
        if (length_status == read_status::error) {
            in.error().stage(detail::read_stage::length);
            return length_status;
        }

        const unsigned char* data = nullptr;
        const auto view_status = in.read_view(static_cast<std::size_t>(length), data);
        if (view_status == read_status::error) {
            in.error().stage(detail::read_stage::length);
            return view_status;
        }

        reader<contract::io::window_input> nested{
            contract::io::window_input{data, static_cast<std::size_t>(length)},
            in.opt_};

        std::size_t element_index = 0;
        while (nested.has_remaining()) {
            if (element_index >= Size) {
                in.error().code(read_error_code::invalid_size)
                    .stage(detail::read_stage::field_value)
                    .element_index(element_index, Size)
                    .sizes(Size, element_index + 1);
                return read_status::error;
            }

            if (codec<value_type>::read(nested, value[element_index]) == read_status::error) {
                in.error(nested.error(), static_cast<std::size_t>(length))
                    .element_index(element_index, Size);
                return read_status::error;
            }
            ++element_index;
        }

        if (element_index != Size) {
            in.error().code(read_error_code::invalid_size)
                .stage(detail::read_stage::field_value)
                .element_index(element_index, Size)
                .sizes(Size, element_index);
            return read_status::error;
        }

        return read_status::ok;
    }

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
};

} // namespace contract::adapters::protobuf
