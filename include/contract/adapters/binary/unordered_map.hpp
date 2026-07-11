#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <unordered_map>

namespace contract::adapters::binary {

template<class K, class V, class Hash, class KeyEqual, class Allocator>
struct codec<std::unordered_map<K, V, Hash, KeyEqual, Allocator>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
        return write(out, base::NoField{}, value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
        if (const auto limit = attributes::max_items_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_items_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }

        const std::size_t size = value.size();
        if (out.write_value(size) == write_status::error) {
            return write_status::error;
        }

        std::size_t index = 0;
        for (const auto& item : value) {
            if (out.write_value(item.first) == write_status::error ||
                out.write_value(item.second) == write_status::error) {
                return out.error()
                    .element_index(index, size)
                    .stage(write_stage::container);
            }
            ++index;
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
        return read(in, base::NoField{}, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
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

        value.clear();
        value.reserve(size);

        for (std::size_t i = 0; i < size; ++i) {
            K key{};
            V mapped{};
            if (in.read_value(key) == read_status::error ||
                in.read_value(mapped) == read_status::error) {
                return in.error()
                    .element_index(i, size)
                    .stage(read_stage::container);
            }

            auto [it, inserted] = value.emplace(std::move(key), std::move(mapped));
            if (!inserted) {
                return in.error()
                    .code(read_error_code::duplicate_key)
                    .element_index(i, size)
                    .stage(read_stage::container);
            }
        }
        return read_status::ok;
    }

};

} // namespace contract::adapters::binary
