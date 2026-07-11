#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <bitset>

namespace contract::adapters::console {

template<std::size_t N>
struct codec<std::bitset<N>, void> {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer& out, const std::bitset<N>& value) {
        out.write_bitset(value);
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::bitset<N>& value) {
        out.write_count_comment("set", value.count(), N > out.max_string_length());
    }

};

} // namespace contract::adapters::console
