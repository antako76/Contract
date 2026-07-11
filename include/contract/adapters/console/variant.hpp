#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <variant>

namespace contract::adapters::console {

template<class... Ts>
struct codec<std::variant<Ts...>, void> {
    static constexpr bool block = true;

    template<class Writer>
    static void write(Writer& out, const std::variant<Ts...>& value) {
        std::visit([&](const auto& item) {
            using item_type = contract::adapters::base::clean_t<decltype(item)>;
            out.write_indexed_item(
                value.index(),
                item,
                contract::adapters::debug::type_name<item_type>()
            );
        }, value);
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::variant<Ts...>&) {
        out.write_count_comment("size", sizeof...(Ts));
    }

};

} // namespace contract::adapters::console
