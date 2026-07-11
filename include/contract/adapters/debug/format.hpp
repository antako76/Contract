#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base/format.hpp>

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace contract::adapters::debug {

struct color_palette {
    std::string_view field_name = "\x1b[36m";
    std::string_view type_name = "\x1b[96m";
    std::string_view string_value = "\x1b[92m";
    std::string_view comment_field_id = "\x1b[96m";
    std::string_view comment_provenance = "\x1b[94m";
    std::string_view comment_attribute = "\x1b[32m";
    std::string_view comment_security = "\x1b[35m";
    std::string_view number_value = "\x1b[33m";
    std::string_view comment = "\x1b[90m";
    std::string_view truncation = "\x1b[31m";
    std::string_view reset = "\x1b[0m";
};

struct color_options {
    bool enabled = false;
    color_palette palette{};
};

struct truncated_string {
    std::string value;
    std::size_t omitted = 0;

    [[nodiscard]] constexpr bool truncated() const {
        return omitted != 0;
    }
};

inline truncated_string truncate_string(std::string_view value, std::size_t max_length) {
    if (value.size() <= max_length) {
        return {std::string(value), 0};
    }

    return {std::string(value.substr(0, max_length)), value.size() - max_length};
}

inline std::string format_bytes_preview(const void* data, std::size_t size, std::size_t max_bytes) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    const std::size_t shown = size < max_bytes ? size : max_bytes;

    std::ostringstream out;
    out << "bytes[" << size << "]";

    for (std::size_t i = 0; i < shown; ++i) {
        out << ' '
            << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(bytes[i]);
    }

    if (shown != size) {
        out << " ...";
    }

    return out.str();
}

template<class Byte>
inline unsigned int byte_to_hex(Byte byte) {
    using value_type = std::remove_cv_t<Byte>;

    if constexpr (std::is_same_v<value_type, std::byte>) {
        return std::to_integer<unsigned int>(byte);
    } else {
        return static_cast<unsigned int>(static_cast<unsigned char>(byte));
    }
}

template<class Byte>
inline bool is_printable_ascii(const Byte* data, std::size_t size) {
    static_assert(sizeof(Byte) == 1, "is_printable_ascii requires byte-sized elements");

    for (std::size_t i = 0; i < size; ++i) {
        const auto value = byte_to_hex(data[i]);
        if (value < 0x20 || value > 0x7e) {
            return false;
        }
    }
    return true;
}

template<class Byte>
inline std::string format_bytes_preview_hex(const Byte* data, std::size_t size, std::size_t max_bytes) {
    static_assert(sizeof(Byte) == 1, "format_bytes_preview_hex requires byte-sized elements");

    const std::size_t shown = size < max_bytes ? size : max_bytes;

    std::ostringstream out;
    out << std::hex << std::nouppercase << std::setfill('0');

    for (std::size_t i = 0; i < shown; ++i) {
        if (i != 0) {
            out << ' ';
        }

        out << std::setw(2) << byte_to_hex(data[i]);
    }

    if (shown != size) {
        if (shown != 0) {
            out << ' ';
        }
        out << "...";
    }

    return out.str();
}

} // namespace contract::adapters::debug
