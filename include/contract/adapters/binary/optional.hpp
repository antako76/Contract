#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <optional>

namespace contract::adapters::binary {

template<class T>
struct codec<std::optional<T>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::optional<T>& value) {
        const bool has_value = value.has_value();
        if (out.write_value(has_value) == write_status::error) {
            return write_status::error;
        }

        if (has_value) {
            return out.write_value(*value);
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, std::optional<T>& value) {
        bool has_value = false;
        if (in.read_value(has_value) == read_status::error) {
            return read_status::error;
        }

        if (has_value) {
            T item{};
            if (in.read_value(item) == read_status::error) {
                return read_status::error;
            }
            value = std::move(item);
        } else {
            value.reset();
        }
        return read_status::ok;
    }

};

} // namespace contract::adapters::binary
