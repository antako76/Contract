#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <optional>

namespace contract::adapters::console {

template<class T>
struct codec<std::optional<T>, void> {
    static constexpr bool block = console::codec<T>::block;

    template<class Writer>
    static void write(Writer& out, const std::optional<T>& value) {
        if (value.has_value()) {
            out.write_value(*value);
        } else {
            if constexpr (block) {
                out.indent();
            }

            out.write("nullopt");

            if constexpr (block) {
                out.newline();
            }
        }
    }

};

} // namespace contract::adapters::console
