#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <unordered_map>

namespace contract::adapters::json {

template<class K, class V, class Hash, class KeyEq, class Allocator>
struct codec<std::unordered_map<K, V, Hash, KeyEq, Allocator>, void> {
    template<class Writer>
    static void write(Writer& out, const std::unordered_map<K, V, Hash, KeyEq, Allocator>& value) {
        out.begin_array();
        for (const auto& item : value) {
            out.begin_value();
            out.begin_array();
            out.begin_value();
            out << item.first;
            out.begin_value();
            out << item.second;
            out.end_array();
        }
        out.end_array();
    }

};

} // namespace contract::adapters::json
