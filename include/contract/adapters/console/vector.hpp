#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>

#include <vector>

namespace contract::adapters::console {

template<class T>
struct codec<std::vector<T>, void> {
    static constexpr bool block = !codec_detail::is_byte_blob_v<T>;

    template<class Writer>
    static void write(Writer& out, const std::vector<T>& value) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_bytes_preview(value.data(), value.size());
        } else {
            out.write(value, detail::node_kind::sequence);
        }
    }

    template<class Writer>
    static void write_comment(Writer& out, const std::vector<T>& value) {
        if constexpr (codec_detail::is_byte_blob_v<T>) {
            out.write_count_comment("bytes", value.size(), out.bytes_preview_truncated(value.data(), value.size()));
        } else {
            out.write_count_comment("size", value.size());
        }
    }

};

} // namespace contract::adapters::console
