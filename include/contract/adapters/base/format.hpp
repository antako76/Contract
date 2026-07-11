#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <string>
#include <string_view>

namespace contract::adapters::base {

inline std::string escape_string(std::string_view value) {
    std::string out;
    out.reserve(value.size());

    constexpr char hex[] = "0123456789abcdef";

    for (unsigned char ch : value) {
        switch (ch) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (ch < 0x20 || ch == 0x7f) {
                out += "\\x";
                out += hex[(ch >> 4) & 0x0f];
                out += hex[ch & 0x0f];
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }

    return out;
}

inline std::string quoted_string(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    out += escape_string(value);
    out.push_back('"');
    return out;
}

} // namespace contract::adapters::base
