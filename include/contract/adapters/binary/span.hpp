#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#if defined(CONTRACT_ENABLE_SPAN) && CONTRACT_ENABLE_SPAN

#include <span>

namespace contract::adapters::binary {

template<class T, std::size_t Extent>
struct codec<std::span<T, Extent>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::span<T, Extent>& value) {
        using element_type = std::remove_cv_t<T>;
        const std::size_t size = value.size();

        if constexpr (Extent == std::dynamic_extent) {
            if (out.write_value(size) == write_status::error) {
                return write_status::error;
            }
        }

        if constexpr (codec_detail::is_raw_codec_value_v<element_type>) {
            if (size != 0) {
                return out.write(value.data(), size * sizeof(element_type));
            }
        } else {
            for (std::size_t i = 0; i < size; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, size)
                        .stage(write_stage::container);
                }
            }
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::span<T, Extent>& value) {
        using element_type = std::remove_cv_t<T>;
        const std::size_t size = value.size();
        const std::size_t bytes = codec_detail::is_raw_codec_value_v<element_type>
            ? size * sizeof(element_type)
            : 0;
        if (const auto limit = attributes::max_items_limit(field); limit && size > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && bytes > *limit) {
            return out.error()
                .code(write_error_code::max_bytes_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, bytes);
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::span<T, Extent> value) {
        using element_type = std::remove_cv_t<T>;
        static_assert(!std::is_const_v<T>,
            "binary::codec<std::span<const T>> cannot read into const span storage");

        std::size_t size = 0;
        if constexpr (Extent == std::dynamic_extent) {
            if (in.read_value(size) == read_status::error) {
                return read_status::error;
            }
            if (value.size() != size) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .stage(read_stage::span)
                    .sizes(size, value.size());
            }
        } else {
            size = Extent;
            if (value.size() != Extent) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .stage(read_stage::span)
                    .sizes(Extent, value.size());
            }
        }

        if constexpr (codec_detail::is_raw_codec_value_v<element_type>) {
            if (size != 0) {
                return in.read(value.data(), size * sizeof(element_type));
            }
        } else {
            for (std::size_t i = 0; i < size; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .element_index(i, size)
                        .stage(read_stage::container);
                }
            }
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::span<T, Extent> value) {
        using element_type = std::remove_cv_t<T>;
        std::size_t size = 0;
        if constexpr (Extent == std::dynamic_extent) {
            if (in.read_value(size) == read_status::error) {
                return read_status::error;
            }
            if (value.size() != size) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(size, value.size());
            }
        } else {
            size = Extent;
            if (value.size() != Extent) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(Extent, value.size());
            }
        }

        const std::size_t bytes = codec_detail::is_raw_codec_value_v<element_type>
            ? size * sizeof(element_type)
            : 0;
        if (const auto limit = attributes::max_items_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && bytes > *limit) {
            return in.error()
                .code(read_error_code::max_bytes_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, bytes);
        }

        if constexpr (codec_detail::is_raw_codec_value_v<element_type>) {
            if (size != 0) {
                return in.read(value.data(), size * sizeof(element_type));
            }
        } else {
            for (std::size_t i = 0; i < size; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .element_index(i, size)
                        .stage(read_stage::container);
                }
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::binary

#endif
