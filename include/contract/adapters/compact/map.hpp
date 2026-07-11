#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <cstdint>
#include <limits>
#include <map>
#include <utility>

namespace contract::adapters::compact {

template<class K, class V, class Compare, class Allocator>
struct codec<std::map<K, V, Compare, Allocator>, void> {
    using map_type = std::map<K, V, Compare, Allocator>;

    template<class Writer>
    static write_status write(Writer& out, const map_type& value) {
        if (out.write_size_header(value_kind::map, value.size()) == write_status::error) {
            return out.error().stage(write_stage::map);
        }

        std::size_t index = 0;
        for (const auto& item : value) {
            if (out.write_value(item.first) == write_status::error ||
                out.write_value(item.second) == write_status::error) {
                return out.error()
                    .element_index(index, value.size())
                    .stage(write_stage::map);
            }
            ++index;
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const map_type& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::map)
                .sizes(*limit, value.size());
        }
        if (out.write_size_header(value_kind::map, value.size()) == write_status::error) {
            return out.error().field(field).stage(write_stage::map);
        }

        std::size_t index = 0;
        for (const auto& item : value) {
            if (out.write_value(item.first) == write_status::error ||
                out.write_value(item.second) == write_status::error) {
                return out.error()
                    .field(field)
                    .element_index(index, value.size())
                    .stage(write_stage::map);
            }
            ++index;
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, map_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return read_status::error;
        }
        return read_items(in, count, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, map_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return in.error().field(field).stage(read_stage::map);
        }
        if (const auto limit = attributes::max_items_limit(field); limit && count > *limit) {
            return in.error()
                .code(read_error_code::max_items_exceeded)
                .field(field)
                .stage(read_stage::map)
                .sizes(*limit, count);
        }
        return read_items(in, field, count, value);
    }

private:
    template<class Reader>
    static read_status read_count(Reader& in, std::size_t& count) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::map, size) == read_status::error) {
            return in.error().stage(read_stage::map);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::map);
        }
        count = static_cast<std::size_t>(size);
        return read_status::ok;
    }

    template<class Reader>
    static read_status read_items(Reader& in, std::size_t count, map_type& value) {
        value.clear();

        for (std::size_t i = 0; i < count; ++i) {
            K key{};
            V mapped{};
            if (in.read_value(key) == read_status::error ||
                in.read_value(mapped) == read_status::error) {
                return in.error()
                    .element_index(i, count)
                    .stage(read_stage::map);
            }

            auto [it, inserted] = value.emplace(std::move(key), std::move(mapped));
            if (!inserted) {
                return in.error()
                    .code(read_error_code::duplicate_key)
                    .element_index(i, count)
                    .stage(read_stage::map);
            }
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read_items(Reader& in, const Field& field, std::size_t count, map_type& value) {
        value.clear();

        for (std::size_t i = 0; i < count; ++i) {
            K key{};
            V mapped{};
            if (in.read_value(key) == read_status::error ||
                in.read_value(mapped) == read_status::error) {
                return in.error()
                    .field(field)
                    .element_index(i, count)
                    .stage(read_stage::map);
            }

            auto [it, inserted] = value.emplace(std::move(key), std::move(mapped));
            if (!inserted) {
                return in.error()
                    .code(read_error_code::duplicate_key)
                    .field(field)
                    .element_index(i, count)
                    .stage(read_stage::map);
            }
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::compact
