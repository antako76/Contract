#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <vector>

namespace contract::adapters::json {

template<class T>
struct codec<std::vector<T>, void> {
    template<class Writer>
    static void write(Writer& out, const std::vector<T>& value) {
        out.begin_array();
        for (const auto& item : value) {
            out.begin_value();
            out << item;
        }
        out.end_array();
    }

};

} // namespace contract::adapters::json
