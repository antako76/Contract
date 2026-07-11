#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <cstdint>
#include <limits>
#include <utility>
#include <variant>

namespace contract::adapters::compact {

template<class... Ts>
struct codec<std::variant<Ts...>, void> {
    using variant_type = std::variant<Ts...>;

    template<class Writer>
    static write_status write(Writer& out, const variant_type& value) {
        if (out.write_size_header(value_kind::array, 2) == write_status::error) {
            return out.error().stage(write_stage::variant);
        }

        const std::size_t index = value.index();
        if (out.write_value(index) == write_status::error) {
            return out.error().stage(write_stage::variant);
        }

        write_status status = write_status::ok;
        std::visit(
            [&](const auto& item) {
                status = out.write_value(item);
            },
            value);
        return status == write_status::error ? out.error().stage(write_stage::variant) : status;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const variant_type& value) {
        if (out.write_size_header(value_kind::array, 2) == write_status::error) {
            return out.error().field(field).stage(write_stage::variant);
        }

        const std::size_t index = value.index();
        if (out.write_value(index) == write_status::error) {
            return out.error().field(field).stage(write_stage::variant);
        }

        write_status status = write_status::ok;
        std::visit(
            [&](const auto& item) {
                status = out.write_value(item);
            },
            value);
        if (status == write_status::error) {
            return out.error().field(field).stage(write_stage::variant);
        }
        return status;
    }

    template<class Reader>
    static read_status read(Reader& in, variant_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return read_status::error;
        }
        if (count != 2) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::variant)
                .sizes(2, count);
        }

        std::size_t index = 0;
        if (in.read_value(index) == read_status::error) {
            return in.error().stage(read_stage::variant);
        }
        return read_by_index<0>(in, value, index);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, variant_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return in.error().field(field).stage(read_stage::variant);
        }
        if (count != 2) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::variant)
                .sizes(2, count);
        }

        std::size_t index = 0;
        if (in.read_value(index) == read_status::error) {
            return in.error().field(field).stage(read_stage::variant);
        }
        return read_by_index<0>(in, field, value, index);
    }

private:
    template<class Reader>
    static read_status read_count(Reader& in, std::size_t& count) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().stage(read_stage::variant);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::variant);
        }
        count = static_cast<std::size_t>(size);
        return read_status::ok;
    }

    template<std::size_t I = 0, class Reader>
    static read_status read_by_index(Reader& in, variant_type& value, std::size_t index) {
        if constexpr (I < sizeof...(Ts)) {
            if (index == I) {
                using alt_type = std::variant_alternative_t<I, variant_type>;
                alt_type item{};
                if (in.read_value(item) == read_status::error) {
                    return in.error()
                        .element_index(1, 2)
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

    template<std::size_t I = 0, class Reader, class Field>
    static read_status read_by_index(Reader& in, const Field& field, variant_type& value, std::size_t index) {
        if constexpr (I < sizeof...(Ts)) {
            if (index == I) {
                using alt_type = std::variant_alternative_t<I, variant_type>;
                alt_type item{};
                if (in.read_value(item) == read_status::error) {
                    return in.error()
                        .field(field)
                        .element_index(1, 2)
                        .stage(read_stage::variant);
                }
                value = std::move(item);
                return read_status::ok;
            }

            return read_by_index<I + 1>(in, field, value, index);
        } else {
            return in.error()
                .code(read_error_code::variant_index_out_of_range)
                .field(field)
                .stage(read_stage::variant)
                .sizes(sizeof...(Ts), index);
        }
    }
};

} // namespace contract::adapters::compact
