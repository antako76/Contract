#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace contract::adapters::compact {

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::array<T, N>& value) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            const std::size_t trimmed = detail::trim_trailing_zeros(value.data(), N);
            if (out.write_size_header(value_kind::bytes, trimmed) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            if (trimmed != 0 && out.write(value.data(), trimmed * sizeof(T)) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, N) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            for (std::size_t i = 0; i < N; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, N)
                        .stage(write_stage::array);
                }
            }
            return write_status::ok;
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::array<T, N>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && N > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::array)
                .sizes(*limit, N);
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            const std::size_t trimmed = detail::trim_trailing_zeros(value.data(), N);
            if (out.write_size_header(value_kind::bytes, trimmed) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            if (trimmed != 0 && out.write(value.data(), trimmed * sizeof(T)) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, N) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            for (std::size_t i = 0; i < N; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .field(field)
                        .element_index(i, N)
                        .stage(write_stage::array);
                }
            }
            return write_status::ok;
        }
    }

    template<class Reader>
    static read_status read(Reader& in, std::array<T, N>& value) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, value);
        } else {
            std::size_t count = 0;
            if (read_count(in, count) == read_status::error) {
                return read_status::error;
            }
            if (count != N) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .stage(read_stage::array)
                    .sizes(N, count);
            }
            return read_items(in, value);
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::array<T, N>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && N > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::array)
                .sizes(*limit, N);
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, field, value);
        } else {
            std::size_t count = 0;
            if (read_count(in, count) == read_status::error) {
                return in.error().field(field).stage(read_stage::array);
            }
            if (count != N) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .field(field)
                    .stage(read_stage::array)
                    .sizes(N, count);
            }
            return read_items(in, field, value);
        }
    }

private:
    template<class Reader>
    static read_status read_bytes(Reader& in, std::array<T, N>& value) {
        std::uint64_t byte_count = 0;
        if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (byte_count > N * sizeof(T)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array)
                .sizes(N * sizeof(T), byte_count);
        }
        if (byte_count != 0 && in.read(value.data(), byte_count) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (byte_count != N * sizeof(T)) {
            std::memset(value.data() + byte_count, 0, N * sizeof(T) - byte_count);
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read_bytes(Reader& in, const Field& field, std::array<T, N>& value) {
        std::uint64_t byte_count = 0;
        if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (byte_count > N * sizeof(T)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::array)
                .sizes(N * sizeof(T), byte_count);
        }
        if (byte_count != 0 && in.read(value.data(), byte_count) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (byte_count != N * sizeof(T)) {
            std::memset(value.data() + byte_count, 0, N * sizeof(T) - byte_count);
        }
        return read_status::ok;
    }

    template<class Reader>
    static read_status read_count(Reader& in, std::size_t& count) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array);
        }
        count = static_cast<std::size_t>(size);
        return read_status::ok;
    }

    template<class Reader>
    static read_status read_items(Reader& in, std::array<T, N>& value) {
        for (std::size_t i = 0; i < N; ++i) {
            if (in.read_value(value[i]) == read_status::error) {
                return in.error()
                    .element_index(i, N)
                    .stage(read_stage::array);
            }
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read_items(Reader& in, const Field& field, std::array<T, N>& value) {
        for (std::size_t i = 0; i < N; ++i) {
            if (in.read_value(value[i]) == read_status::error) {
                return in.error()
                    .field(field)
                    .element_index(i, N)
                    .stage(read_stage::array);
            }
        }
        return read_status::ok;
    }
};

template<class T, std::size_t Size>
struct codec<T[Size], void> {
    template<class Writer>
    static write_status write(Writer& out, const T (&value)[Size]) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            const std::size_t trimmed = detail::trim_trailing_zeros(value, Size);
            if (out.write_size_header(value_kind::bytes, trimmed) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            if (trimmed != 0 && out.write(value, trimmed * sizeof(T)) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, Size) == write_status::error) {
                return out.error().stage(write_stage::array);
            }
            for (std::size_t i = 0; i < Size; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, Size)
                        .stage(write_stage::array);
                }
            }
            return write_status::ok;
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T (&value)[Size]) {
        if (const auto limit = attributes::max_items_limit(field); limit && Size > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::array)
                .sizes(*limit, Size);
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            const std::size_t trimmed = detail::trim_trailing_zeros(value, Size);
            if (out.write_size_header(value_kind::bytes, trimmed) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            if (trimmed != 0 && out.write(value, trimmed * sizeof(T)) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, Size) == write_status::error) {
                return out.error().field(field).stage(write_stage::array);
            }
            for (std::size_t i = 0; i < Size; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .field(field)
                        .element_index(i, Size)
                        .stage(write_stage::array);
                }
            }
            return write_status::ok;
        }
    }

    template<class Reader>
    static read_status read(Reader& in, T (&value)[Size]) {
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, value);
        } else {
            std::size_t count = 0;
            if (read_count(in, count) == read_status::error) {
                return read_status::error;
            }
            if (count != Size) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .stage(read_stage::array)
                    .sizes(Size, count);
            }
            return read_items(in, value);
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, T (&value)[Size]) {
        if (const auto limit = attributes::max_items_limit(field); limit && Size > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::array)
                .sizes(*limit, Size);
        }
        if constexpr (detail::is_byte_like_element_v<T>) {
            return read_bytes(in, field, value);
        } else {
            std::size_t count = 0;
            if (read_count(in, count) == read_status::error) {
                return in.error().field(field).stage(read_stage::array);
            }
            if (count != Size) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .field(field)
                    .stage(read_stage::array)
                    .sizes(Size, count);
            }
            return read_items(in, field, value);
        }
    }

private:
    template<class Reader>
    static read_status read_bytes(Reader& in, T (&value)[Size]) {
        std::uint64_t byte_count = 0;
        if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (byte_count > Size * sizeof(T)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array)
                .sizes(Size * sizeof(T), byte_count);
        }
        if (byte_count != 0 && in.read(value, byte_count) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (byte_count != Size * sizeof(T)) {
            std::memset(value + byte_count, 0, Size * sizeof(T) - byte_count);
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read_bytes(Reader& in, const Field& field, T (&value)[Size]) {
        std::uint64_t byte_count = 0;
        if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (byte_count > Size * sizeof(T)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::array)
                .sizes(Size * sizeof(T), byte_count);
        }
        if (byte_count != 0 && in.read(value, byte_count) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (byte_count != Size * sizeof(T)) {
            std::memset(value + byte_count, 0, Size * sizeof(T) - byte_count);
        }
        return read_status::ok;
    }

    template<class Reader>
    static read_status read_count(Reader& in, std::size_t& count) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array);
        }
        count = static_cast<std::size_t>(size);
        return read_status::ok;
    }

    template<class Reader>
    static read_status read_items(Reader& in, T (&value)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) {
            if (in.read_value(value[i]) == read_status::error) {
                return in.error()
                    .element_index(i, Size)
                    .stage(read_stage::array);
            }
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read_items(Reader& in, const Field& field, T (&value)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) {
            if (in.read_value(value[i]) == read_status::error) {
                return in.error()
                    .field(field)
                    .element_index(i, Size)
                    .stage(read_stage::array);
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::compact
