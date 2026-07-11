#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <cstdint>
#include <limits>
#include <span>

namespace contract::adapters::compact {

template<class T, std::size_t Extent>
struct codec<std::span<T, Extent>, void> {
    using element_type = std::remove_cv_t<T>;

    template<class Writer>
    static write_status write(Writer& out, const std::span<T, Extent>& value) {
        const std::size_t count = value.size();
        if constexpr (detail::is_byte_like_element_v<element_type>) {
            if (out.write_size_header(value_kind::bytes, count) == write_status::error) {
                return out.error().stage(write_stage::span);
            }
            if (count != 0 && out.write(value.data(), count * sizeof(element_type)) == write_status::error) {
                return out.error().stage(write_stage::span);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, count) == write_status::error) {
                return out.error().stage(write_stage::span);
            }
            for (std::size_t i = 0; i < count; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .element_index(i, count)
                        .stage(write_stage::span);
                }
            }
            return write_status::ok;
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::span<T, Extent>& value) {
        const std::size_t count = value.size();
        if (const auto limit = attributes::max_items_limit(field); limit && count > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::span)
                .sizes(*limit, count);
        }
        if constexpr (detail::is_byte_like_element_v<element_type>) {
            if (out.write_size_header(value_kind::bytes, count) == write_status::error) {
                return out.error().field(field).stage(write_stage::span);
            }
            if (count != 0 && out.write(value.data(), count * sizeof(element_type)) == write_status::error) {
                return out.error().field(field).stage(write_stage::span);
            }
            return write_status::ok;
        } else {
            if (out.write_size_header(value_kind::array, count) == write_status::error) {
                return out.error().field(field).stage(write_stage::span);
            }
            for (std::size_t i = 0; i < count; ++i) {
                if (out.write_value(value[i]) == write_status::error) {
                    return out.error()
                        .field(field)
                        .element_index(i, count)
                        .stage(write_stage::span);
                }
            }
            return write_status::ok;
        }
    }

    template<class Reader>
    static read_status read(Reader& in, std::span<T, Extent> value) {
        static_assert(!std::is_const_v<T>,
            "compact::codec<std::span<const T>> cannot read into const span storage");

        const std::size_t expected_count = value.size();
        if constexpr (detail::is_byte_like_element_v<element_type>) {
            std::uint64_t byte_count = 0;
            if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
                return in.error().stage(read_stage::span);
            }
            if (byte_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .stage(read_stage::span);
            }
            const auto count = static_cast<std::size_t>(byte_count);
            if (count != expected_count * sizeof(element_type)) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .stage(read_stage::span)
                    .sizes(expected_count * sizeof(element_type), count);
            }
            if (count != 0 && in.read(value.data(), count) == read_status::error) {
                return in.error().stage(read_stage::span);
            }
            return read_status::ok;
        } else {
            std::uint64_t size = 0;
            if (in.read_size_header(value_kind::array, size) == read_status::error) {
                return in.error().stage(read_stage::span);
            }
            if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .stage(read_stage::span);
            }
            const auto count = static_cast<std::size_t>(size);
            if (count != expected_count) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .stage(read_stage::span)
                    .sizes(expected_count, count);
            }
            for (std::size_t i = 0; i < count; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .element_index(i, count)
                        .stage(read_stage::span);
                }
            }
            return read_status::ok;
        }
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::span<T, Extent> value) {
        static_assert(!std::is_const_v<T>,
            "compact::codec<std::span<const T>> cannot read into const span storage");

        const std::size_t expected_count = value.size();
        if constexpr (detail::is_byte_like_element_v<element_type>) {
            std::uint64_t byte_count = 0;
            if (in.read_size_header(value_kind::bytes, byte_count) == read_status::error) {
                return in.error().field(field).stage(read_stage::span);
            }
            if (byte_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .field(field)
                    .stage(read_stage::span);
            }
            const auto count = static_cast<std::size_t>(byte_count);
            if (const auto limit = attributes::max_items_limit(field); limit && count > *limit) {
                return in.error()
                    .code(read_error_code::max_items_exceeded)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(*limit, count);
            }
            if (count != expected_count * sizeof(element_type)) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(expected_count * sizeof(element_type), count);
            }
            if (count != 0 && in.read(value.data(), count) == read_status::error) {
                return in.error().field(field).stage(read_stage::span);
            }
            return read_status::ok;
        } else {
            std::uint64_t size = 0;
            if (in.read_size_header(value_kind::array, size) == read_status::error) {
                return in.error().field(field).stage(read_stage::span);
            }
            if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                return in.error()
                    .code(read_error_code::invalid_size)
                    .field(field)
                    .stage(read_stage::span);
            }
            const auto count = static_cast<std::size_t>(size);
            if (const auto limit = attributes::max_items_limit(field); limit && count > *limit) {
                return in.error()
                    .code(read_error_code::max_items_exceeded)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(*limit, count);
            }
            if (count != expected_count) {
                return in.error()
                    .code(read_error_code::span_size_mismatch)
                    .field(field)
                    .stage(read_stage::span)
                    .sizes(expected_count, count);
            }
            for (std::size_t i = 0; i < count; ++i) {
                if (in.read_value(value[i]) == read_status::error) {
                    return in.error()
                        .field(field)
                        .element_index(i, count)
                        .stage(read_stage::span);
                }
            }
            return read_status::ok;
        }
    }
};

} // namespace contract::adapters::compact
