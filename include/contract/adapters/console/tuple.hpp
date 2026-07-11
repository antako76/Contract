#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <tuple>

namespace contract::adapters::console {

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    static constexpr bool block = true;

    template<class Writer>
    static void write(Writer& out, const std::tuple<Ts...>& value) {
        write_items(out, value, std::index_sequence_for<Ts...>{});
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::tuple<Ts...>&) {
        out.write_count_comment("size", sizeof...(Ts));
    }

private:
    template<class Writer, std::size_t... Is>
    static void write_items(Writer& out, const std::tuple<Ts...>& value, std::index_sequence<Is...>) {
        (out.write_indexed_item(
            Is,
            std::get<Is>(value),
            contract::adapters::debug::type_name<contract::adapters::base::clean_t<decltype(std::get<Is>(value))>>()
        ), ...);
    }

};

} // namespace contract::adapters::console
