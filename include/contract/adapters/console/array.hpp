#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <array>

namespace contract::adapters::console {

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    static constexpr bool block = !codec_detail::is_byte_blob_v<T>;

    template<class Writer>
    static void write(Writer& out, const std::array<T, N>& value) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_bytes_preview(value.data(), value.size());
        } else {
            out.write(value, detail::node_kind::sequence);
        }
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::array<T, N>& value) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_count_comment("bytes", N, out.bytes_preview_truncated(value.data(), N));
        } else {
            out.write_count_comment("size", N);
        }
    }

};

template<class T, std::size_t N>
struct codec<T[N], void> {
    static constexpr bool block = !codec_detail::is_byte_blob_v<T>;

    template<class Writer>
    static void write(Writer& out, const T (&value)[N]) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_bytes_preview(value, N);
        } else {
            out.write(value, detail::node_kind::sequence);
        }
    }

    template<class Writer>
    static void write_comment(Writer& out, const T (&value)[N]) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_count_comment("bytes", N, out.bytes_preview_truncated(value, N));
        } else {
            out.write_count_comment("size", N);
        }
    }

};

} // namespace contract::adapters::console
