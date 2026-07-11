#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace contract::adapters::compact {

template<class T>
struct codec<std::vector<T>, void> {
    using value_type = contract::adapters::base::clean_t<T>;

    template<class Writer>
    static write_status write(Writer& out, const std::vector<T>& value) {
        if (out.write_size_header(value_kind::array, value.size()) == write_status::error) {
            return out.error().stage(write_stage::array);
        }
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (out.write_value(value[i]) == write_status::error) {
                return out.error()
                    .element_index(i, value.size())
                    .stage(write_stage::array);
            }
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::vector<T>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::array)
                .sizes(*limit, value.size());
        }
        if (out.write_size_header(value_kind::array, value.size()) == write_status::error) {
            return out.error().field(field).stage(write_stage::array);
        }
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (out.write_value(value[i]) == write_status::error) {
                return out.error()
                    .field(field)
                    .element_index(i, value.size())
                    .stage(write_stage::array);
            }
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::vector<T>& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array);
        }

        const auto count = static_cast<std::size_t>(size);
        value.clear();
        value.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            value_type item{};
            if (in.read_value(item) == read_status::error) {
                return in.error()
                    .element_index(i, count)
                    .stage(read_stage::array);
            }
            value.push_back(std::move(item));
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::vector<T>& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::array);
        }
        if (const auto limit = attributes::max_items_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::array)
                .sizes(*limit, static_cast<std::size_t>(size));
        }

        const auto count = static_cast<std::size_t>(size);
        value.clear();
        value.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            value_type item{};
            if (in.read_value(item) == read_status::error) {
                return in.error()
                    .field(field)
                    .element_index(i, count)
                    .stage(read_stage::array);
            }
            value.push_back(std::move(item));
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::compact
