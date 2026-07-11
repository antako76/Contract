#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary.hpp>

#include <variant>

namespace contract::adapters::binary {

template<class... Ts>
struct codec<std::variant<Ts...>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::variant<Ts...>& value) {
        const std::size_t index = value.index();
        if (out.write_value(index) == write_status::error) {
            return write_status::error;
        }

        write_status status = write_status::ok;
        std::visit([&](const auto& item) {
            status = out.write_value(item);
        }, value);
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, std::variant<Ts...>& value) {
        std::size_t index = 0;
        if (in.read_value(index) == read_status::error) {
            return read_status::error;
        }
        return read_by_index<0>(in, value, index);
    }

private:
    template<std::size_t I = 0, class Reader>
    static read_status read_by_index(Reader& in, std::variant<Ts...>& value, std::size_t index) {
        if constexpr (I < sizeof...(Ts)) {
            if (index == I) {
                using alt_type = std::variant_alternative_t<I, std::variant<Ts...>>;
                alt_type item{};
                if (in.read_value(item) == read_status::error) {
                    return in.error()
                        .element_index(I, sizeof...(Ts))
                        .stage(read_stage::variant);
                }
                value = std::move(item);
                return read_status::ok;
            }

            return read_by_index<I + 1>(in, value, index);
        } else {
            return in.error()
                .code(read_error_code::variant_index_out_of_range)
                .stage(read_stage::variant)
                .sizes(sizeof...(Ts), index);
        }
    }
};

} // namespace contract::adapters::binary
