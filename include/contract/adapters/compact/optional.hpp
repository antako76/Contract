#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

namespace contract::adapters::compact {

template<class T>
struct codec<std::optional<T>, void> {
    using value_type = contract::adapters::base::clean_t<T>;

    template<class Writer>
    static write_status write(Writer& out, const std::optional<T>& value) {
        if (!value.has_value()) {
            return out.write_null();
        }
        return out.write_value(*value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::optional<T>& value) {
        unsigned char header = 0;
        if (in.peek_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (header == detail::null_header) {
            in.consume_byte();
            value.reset();
            return read_status::ok;
        }

        value_type item{};
        if (in.read_value(item) == read_status::error) {
            return read_status::error;
        }
        value = std::move(item);
        return read_status::ok;
    }
};

} // namespace contract::adapters::compact

