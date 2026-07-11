#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <map>

namespace contract::adapters::console {

template<class K, class V, class Compare, class Allocator>
struct codec<std::map<K, V, Compare, Allocator>, void> {
    static constexpr bool block = true;

    template<class Writer>
    static void write(Writer& out, const std::map<K, V, Compare, Allocator>& value) {
        using key_type = contract::adapters::base::clean_t<K>;
        if constexpr (console::codec<key_type>::block) {
            std::size_t index = 0;
            for (const auto& [key, item] : value) {
                out.write_map_entry(index, key, item);
                ++index;
            }
        } else {
            std::size_t index = 0;
            for (const auto& [key, item] : value) {
                out.write_map_item(index, key, item);
                ++index;
            }
        }
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::map<K, V, Compare, Allocator>& value) {
        out.write_count_comment("size", value.size());
    }

};

} // namespace contract::adapters::console
