#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <tuple>

namespace contract::adapters::binary {

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::tuple<Ts...>& value) {
        write_status status = write_status::ok;
        std::apply([&](const auto&... items) {
            ((status == write_status::ok
                ? status = out.write_value(items)
                : status), ...);
        }, value);
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, std::tuple<Ts...>& value) {
        read_status status = read_status::ok;
        std::apply([&](auto&... items) {
            ((status == read_status::ok
                ? status = in.read_value(items)
                : status), ...);
        }, value);
        return status;
    }

};

} // namespace contract::adapters::binary
