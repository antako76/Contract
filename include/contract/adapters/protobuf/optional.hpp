#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf.hpp>

#include <optional>

namespace contract::adapters::protobuf {

template<class T>
struct codec<std::optional<T>, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::optional<T>& value) {
        if (value.has_value()) {
            using value_type = contract::adapters::base::clean_t<T>;
            using codec_type = codec<value_type>;
            return codec_type::write(out, *value);
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::optional<T>& value) {
        if (!value.has_value()) {
            return write_status::ok;
        }
        using value_type = contract::adapters::base::clean_t<T>;
        return codec<value_type>::write(out, field, *value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::optional<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        auto& slot = value.emplace();
        const auto status = codec<value_type>::read(in, slot);
        if (status == read_status::error) {
            return status;
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::optional<T>& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        auto& slot = value.emplace();
        const auto status = codec<value_type>::read(in, field, wire, slot);
        if (status == read_status::error) {
            return status;
        }
        return read_status::ok;
    }
};

} // namespace contract::adapters::protobuf
