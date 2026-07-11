#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>

#include <optional>

namespace contract::adapters::json {

template<class T>
struct codec<std::optional<T>, void> {
    template<class Writer>
    static void write(Writer& out, const std::optional<T>& value) {
        if (value.has_value()) {
            out.write_value(*value);
        } else {
            out.write_null();
        }
    }

};

} // namespace contract::adapters::json
