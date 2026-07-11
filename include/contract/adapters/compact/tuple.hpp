#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact.hpp>

#include <cstdint>
#include <limits>
#include <tuple>

namespace contract::adapters::compact {

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    using tuple_type = std::tuple<Ts...>;

    template<class Writer>
    static write_status write(Writer& out, const tuple_type& value) {
        if (out.write_size_header(value_kind::array, sizeof...(Ts)) == write_status::error) {
            return out.error().stage(write_stage::array);
        }
        return write_items(out, value, std::index_sequence_for<Ts...>{});
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const tuple_type& value) {
        if (out.write_size_header(value_kind::array, sizeof...(Ts)) == write_status::error) {
            return out.error().field(field).stage(write_stage::array);
        }
        return write_items(out, field, value, std::index_sequence_for<Ts...>{});
    }

    template<class Reader>
    static read_status read(Reader& in, tuple_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return read_status::error;
        }
        if (count != sizeof...(Ts)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array)
                .sizes(sizeof...(Ts), count);
        }
        return read_items(in, value, std::index_sequence_for<Ts...>{});
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, tuple_type& value) {
        std::size_t count = 0;
        if (read_count(in, count) == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        if (count != sizeof...(Ts)) {
            return in.error()
                .code(read_error_code::invalid_size)
                .field(field)
                .stage(read_stage::array)
                .sizes(sizeof...(Ts), count);
        }
        return read_items(in, field, value, std::index_sequence_for<Ts...>{});
    }

private:
    template<class Writer, std::size_t... Is>
    static write_status write_items(Writer& out, const tuple_type& value, std::index_sequence<Is...>) {
        write_status status = write_status::ok;
        ((status == write_status::ok
            ? status = out.write_value(std::get<Is>(value))
            : status), ...);
        return status == write_status::error ? out.error().stage(write_stage::array) : status;
    }

    template<class Writer, class Field, std::size_t... Is>
    static write_status write_items(Writer& out, const Field& field, const tuple_type& value,
        std::index_sequence<Is...>) {
        write_status status = write_status::ok;
        ((status == write_status::ok
            ? status = out.write_value(std::get<Is>(value))
            : status), ...);
        if (status == write_status::error) {
            return out.error().field(field).stage(write_stage::array);
        }
        return status;
    }

    template<class Reader>
    static read_status read_count(Reader& in, std::size_t& count) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::array, size) == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::array);
        }
        count = static_cast<std::size_t>(size);
        return read_status::ok;
    }

    template<class Reader, std::size_t... Is>
    static read_status read_items(Reader& in, tuple_type& value, std::index_sequence<Is...>) {
        read_status status = read_status::ok;
        ((status == read_status::ok
            ? status = in.read_value(std::get<Is>(value))
            : status), ...);
        if (status == read_status::error) {
            return in.error().stage(read_stage::array);
        }
        return status;
    }

    template<class Reader, class Field, std::size_t... Is>
    static read_status read_items(Reader& in, const Field& field, tuple_type& value,
        std::index_sequence<Is...>) {
        read_status status = read_status::ok;
        ((status == read_status::ok
            ? status = in.read_value(std::get<Is>(value))
            : status), ...);
        if (status == read_status::error) {
            return in.error().field(field).stage(read_stage::array);
        }
        return status;
    }
};

} // namespace contract::adapters::compact
