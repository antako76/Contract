#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <tuple>

namespace contract::adapters::json {

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    template<class Writer>
    static void write(Writer& out, const std::tuple<Ts...>& value) {
        out.begin_array();
        std::apply([&](const auto&... items) {
            ((out.begin_value(), out << items), ...);
        }, value);
        out.end_array();
    }

};

} // namespace contract::adapters::json
