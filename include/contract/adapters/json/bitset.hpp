#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <bitset>

namespace contract::adapters::json {

template<std::size_t N>
struct codec<std::bitset<N>, void> {
    template<class Writer>
    static void write(Writer& out, const std::bitset<N>& value) {
        out.write_string(value.to_string());
    }

};

} // namespace contract::adapters::json
