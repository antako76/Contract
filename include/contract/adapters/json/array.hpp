#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/adapters/json.hpp>

#include <array>

namespace contract::adapters::json {

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    template<class Writer>
    static void write(Writer& out, const std::array<T, N>& value) {
        if constexpr (std::is_same_v<T, char>) {
            const std::size_t trimmed = contract::adapters::base::trim_trailing_zeros(value.data(), N);
            out.write_string(std::string_view{value.data(), trimmed});
        } else {
            out.begin_array();
            for (const auto& item : value) {
                out.begin_value();
                out << item;
            }
            out.end_array();
        }
    }

};

template<class T, std::size_t N>
struct codec<T[N], void> {
    template<class Writer>
    static void write(Writer& out, const T (&value)[N]) {
        if constexpr (std::is_same_v<T, char>) {
            const std::size_t trimmed = contract::adapters::base::trim_trailing_zeros(value, N);
            out.write_string(std::string_view{value, trimmed});
        } else {
            out.begin_array();
            for (std::size_t i = 0; i < N; ++i) {
                out.begin_value();
                out << value[i];
            }
            out.end_array();
        }
    }

};

} // namespace contract::adapters::json
