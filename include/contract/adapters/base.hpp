#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/tag.hpp>

#include <cstddef>
#include <type_traits>

namespace contract::adapters::base {

enum class status {
    ok,
    error,
};

template<class T>
using clean_t = std::remove_cvref_t<T>;

struct NoField {};

template<class Field>
inline constexpr bool has_field_context_v =
    !std::is_same_v<clean_t<Field>, NoField>;

template<class>
inline constexpr bool always_false_v = false;

template<class Codec, class Writer, class Field, class Value>
concept has_field_write = requires(Writer& writer, const Field& field, const Value& value) {
    Codec::write(writer, field, value);
};

template<class Codec, class Reader, class Field, class Value>
concept has_field_read = requires(Reader& reader, const Field& field, Value& value) {
    Codec::read(reader, field, value);
};

template<class T>
concept has_contract_definition = requires {
    contract_definition(contract::tag<clean_t<T>>{});
};

// Element types whose in-memory bytes ARE their wire representation for a
// fixed-size array (unlike multi-byte integers, which most adapters encode
// as variable-length payloads, not raw memory). Arrays/spans of these can
// bulk-copy as a single byte-blob instead of looping through per-element
// codec dispatch.
template<class T>
inline constexpr bool is_byte_like_element_v =
    std::is_same_v<T, std::byte> ||
    (sizeof(T) == 1 && std::is_integral_v<T> && !std::is_same_v<T, bool>);

// A trailing run of zero bytes in a fixed-size byte-like buffer is elidable:
// those bytes are reconstructed as zero on read regardless of what they
// meant to the writer, so trimming them is lossless for any content, not
// just NUL-padded strings. This is NOT a C-string convention (no scan for an
// embedded NUL, no assumption about what follows it) - only the literal tail
// of the buffer is examined.
template<class T>
inline std::size_t trim_trailing_zeros(const T* data, std::size_t count) noexcept {
    while (count > 0 && data[count - 1] == T{}) {
        --count;
    }
    return count;
}

} // namespace contract::adapters::base
