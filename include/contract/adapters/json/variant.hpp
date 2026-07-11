#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <variant>

namespace contract::adapters::json {

template<class... Ts>
struct codec<std::variant<Ts...>, void> {
    template<class Writer>
    static void write(Writer& out, const std::variant<Ts...>& value) {
        out.begin_array();
        out.begin_value();
        out.write_number(value.index());
        std::visit([&](const auto& item) {
            out.begin_value();
            out << item;
        }, value);
        out.end_array();
    }

};

} // namespace contract::adapters::json
