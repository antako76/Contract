#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <vector>

namespace contract::adapters::binary {

template<class T>
struct codec<std::vector<T>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::vector<T>& value) {
        return write(out, base::NoField{}, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::vector<T>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            const std::size_t bytes = value.size() * sizeof(T);
            if (const auto limit = attributes::max_bytes_limit(field); limit && bytes > *limit) {
                return out.error()
                    .code(write_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(write_stage::attribute_guard)
                    .sizes(*limit, bytes);
            }
        }
        if (out.write_value(value.size()) == write_status::error) {
            return write_status::error;
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            if (!value.empty()) {
                return out.write(value.data(), value.size() * sizeof(T));
            }
        } else {
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, value.size())
                        .stage(write_stage::container);
                }
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::vector<T>& value) {
        return read(in, base::NoField{}, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::vector<T>& value) {
        std::size_t size = 0;
        if (in.read_value(size) == read_status::error) {
            return read_status::error;
        }
        if (const auto limit = attributes::max_items_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            const std::size_t bytes = size * sizeof(T);
            if (const auto limit = attributes::max_bytes_limit(field); limit && bytes > *limit) {
                return in.error()
                    .code(read_error_code::max_bytes_exceeded)
                    .field(field)
                    .stage(read_stage::attribute_guard)
                    .sizes(*limit, bytes);
            }
        }
        if constexpr (codec_detail::is_raw_codec_value_v<T>) {
            value.resize(size);
            if (size != 0) {
                return in.read(value.data(), size * sizeof(T));
            }
        } else {
            value.clear();
            value.reserve(size);

            for (std::size_t i = 0; i < size; ++i) {
                T item{};
                if (in.read_value(item) == read_status::error) {
                    return in.error()
                        .element_index(i, size)
                        .stage(read_stage::container);
                }
                value.push_back(std::move(item));
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::binary
