#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <unordered_map>

namespace contract::adapters::console {

template<class K, class V, class Hash, class KeyEqual, class Allocator>
struct codec<std::unordered_map<K, V, Hash, KeyEqual, Allocator>, void> {
    static constexpr bool block = true;

    template<class Writer>
    static void write(Writer& out, const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
        using key_type = contract::adapters::base::clean_t<K>;
        std::size_t index = 0;

        if constexpr (console::codec<key_type>::block) {
            for (const auto& [key, item] : value) {
                out.write_map_entry(index, key, item);
                ++index;
            }
        } else {
            for (const auto& [key, item] : value) {
                out.write_map_item(index, key, item);
                ++index;
            }
        }
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& value) {
        out.write_count_comment("size", value.size());
    }

};

} // namespace contract::adapters::console
