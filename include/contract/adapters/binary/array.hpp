#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <array>
#include <iterator>

namespace contract::adapters::binary {

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::array<T, N>& value) {
        return write(out, base::NoField{}, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::array<T, N>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && N > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, N);
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            if (const auto limit = attributes::max_bytes_limit(field); limit && N * sizeof(T) > *limit) {
                return out.error()
                    .code(write_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(write_stage::attribute_guard)
                    .sizes(*limit, N * sizeof(T));
            }
            if constexpr (N != 0) {
                return out.write(std::data(value), N * sizeof(T));
            }
        } else {
            for (std::size_t i = 0; i < N; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, N)
                        .stage(write_stage::container);
                }
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::array<T, N>& value) {
        return read(in, base::NoField{}, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::array<T, N>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && N > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, N);
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            if (const auto limit = attributes::max_bytes_limit(field); limit && N * sizeof(T) > *limit) {
                return in.error()
                    .code(read_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(read_stage::attribute_guard)
                    .sizes(*limit, N * sizeof(T));
            }
            if constexpr (N != 0) {
                return in.read(std::data(value), N * sizeof(T));
            }
        } else {
            for (std::size_t i = 0; i < N; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .element_index(i, N)
                        .stage(read_stage::container);
                }
            }
        }
        return read_status::ok;
    }
};

template<class T, std::size_t Size>
struct codec<T[Size], void> {
    template<class Writer>
    static write_status write(Writer& out, const T (&value)[Size]) {
        return write(out, base::NoField{}, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T (&value)[Size]) {
        if (const auto limit = attributes::max_items_limit(field); limit && Size > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, Size);
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            if (const auto limit = attributes::max_bytes_limit(field); limit && Size * sizeof(T) > *limit) {
                return out.error()
                    .code(write_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(write_stage::attribute_guard)
                    .sizes(*limit, Size * sizeof(T));
            }
            if constexpr (Size != 0) {
                return out.write(value, Size * sizeof(T));
            }
        } else {
            for (std::size_t i = 0; i < Size; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, Size)
                        .stage(write_stage::container);
                }
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, T (&value)[Size]) {
        return read(in, base::NoField{}, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, T (&value)[Size]) {
        if (const auto limit = attributes::max_items_limit(field); limit && Size > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, Size);
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            if (const auto limit = attributes::max_bytes_limit(field); limit && Size * sizeof(T) > *limit) {
                return in.error()
                    .code(read_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(read_stage::attribute_guard)
                    .sizes(*limit, Size * sizeof(T));
            }
            if constexpr (Size != 0) {
                return in.read(value, Size * sizeof(T));
            }
        } else {
            for (std::size_t i = 0; i < Size; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .element_index(i, Size)
                        .stage(read_stage::container);
                }
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::binary
