#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0
//
// Native compact tagged binary adapter core.

#include <contract/adapters/base.hpp>
#include <contract/attributes/validation.hpp>
#include <contract/check.hpp>
#include <contract/contract.hpp>
#include <contract/detail/error.hpp>
#include <contract/io/byte.hpp>
#include <contract/io/byte_window.hpp>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace contract::adapters::compact {

struct options {};

struct adapter_traits {
    static constexpr contract::adapter_type type = contract::adapter_type::wire;
    static constexpr contract::attribute_visibility visibility =
        contract::attribute_visibility::declared_vocabularies_only;

    using visible_vocabularies = contract::vocabularies<
        contract::check::vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::default_for<contract::check::vocabulary>::ignore,
        contract::for_tag<contract::check::tag::decode_guard>::enforce>;
};

template<class T, class Enable = void>
struct codec;

using read_status = contract::adapters::base::status;
using write_status = contract::adapters::base::status;

namespace detail {

enum class value_kind : std::uint8_t {
    zero,
    small_uint,
    small_neg,
    int_payload,
    bytes,
    string,
    array,
    map,
    object,
    bool_false,
    bool_true,
    null,
    float32,
    float64,
};

enum class read_error_code {
    unknown,
    input_error,
    truncated,
    invalid_header,
    invalid_size,
    invalid_integer,
    invalid_field_id,
    duplicate_key,
    max_items_exceeded,
    span_size_mismatch,
    variant_index_out_of_range,
    unsupported_operation,
};

enum class write_error_code {
    unknown,
    output_error,
    invalid_size,
    invalid_field_id,
    max_items_exceeded,
    unsupported_operation,
};

enum class read_stage {
    none,
    header,
    integer,
    size,
    raw_bytes,
    array,
    map,
    variant,
    span,
    object,
    field_id,
    field_value,
    skip,
    float32,
    float64,
};

enum class write_stage {
    none,
    header,
    integer,
    size,
    raw_bytes,
    array,
    map,
    variant,
    span,
    object,
    field_id,
    field_value,
    float32,
    float64,
};

inline std::string_view to_string(read_error_code value) noexcept {
    switch (value) {
    case read_error_code::unknown:
        return "unknown";
    case read_error_code::input_error:
        return "input error";
    case read_error_code::truncated:
        return "truncated";
    case read_error_code::invalid_header:
        return "invalid header";
    case read_error_code::invalid_size:
        return "invalid size";
    case read_error_code::invalid_integer:
        return "invalid integer";
    case read_error_code::invalid_field_id:
        return "invalid field id";
    case read_error_code::duplicate_key:
        return "duplicate key";
    case read_error_code::max_items_exceeded:
        return "max items exceeded";
    case read_error_code::span_size_mismatch:
        return "span size mismatch";
    case read_error_code::variant_index_out_of_range:
        return "variant index out of range";
    case read_error_code::unsupported_operation:
        return "unsupported operation";
    }
    return "unknown";
}

inline std::string_view to_string(write_error_code value) noexcept {
    switch (value) {
    case write_error_code::unknown:
        return "unknown";
    case write_error_code::output_error:
        return "output error";
    case write_error_code::invalid_size:
        return "invalid size";
    case write_error_code::invalid_field_id:
        return "invalid field id";
    case write_error_code::max_items_exceeded:
        return "max items exceeded";
    case write_error_code::unsupported_operation:
        return "unsupported operation";
    }
    return "unknown";
}

inline std::string_view to_string(read_stage value) noexcept {
    switch (value) {
    case read_stage::none:
        return "input";
    case read_stage::header:
        return "header";
    case read_stage::integer:
        return "integer";
    case read_stage::size:
        return "size";
    case read_stage::raw_bytes:
        return "raw bytes";
    case read_stage::array:
        return "array";
    case read_stage::map:
        return "map";
    case read_stage::variant:
        return "variant";
    case read_stage::span:
        return "span";
    case read_stage::object:
        return "object";
    case read_stage::field_id:
        return "field id";
    case read_stage::field_value:
        return "field value";
    case read_stage::skip:
        return "skip";
    case read_stage::float32:
        return "float32";
    case read_stage::float64:
        return "float64";
    }
    return "input";
}

inline std::string_view to_string(write_stage value) noexcept {
    switch (value) {
    case write_stage::none:
        return "output";
    case write_stage::header:
        return "header";
    case write_stage::integer:
        return "integer";
    case write_stage::size:
        return "size";
    case write_stage::raw_bytes:
        return "raw bytes";
    case write_stage::array:
        return "array";
    case write_stage::map:
        return "map";
    case write_stage::variant:
        return "variant";
    case write_stage::span:
        return "span";
    case write_stage::object:
        return "object";
    case write_stage::field_id:
        return "field id";
    case write_stage::field_value:
        return "field value";
    case write_stage::float32:
        return "float32";
    case write_stage::float64:
        return "float64";
    }
    return "output";
}

struct read_error : contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status> {
public:
    using contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status>::adapter_error_base;

    std::string message() const {
        return base_message("compact reader", "reading", read_stage::none);
    }

private:
    friend class contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status>;

    void append_adapter_details(std::string&) const {}
};

inline std::string to_string(const read_error& error) {
    return error.message();
}

struct write_error : contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status> {
public:
    using contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status>::adapter_error_base;

    std::string message() const {
        return base_message("compact writer", "writing", write_stage::none);
    }

private:
    friend class contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status>;

    void append_adapter_details(std::string&) const {}
};

inline std::string to_string(const write_error& error) {
    return error.message();
}

inline constexpr unsigned char zero_header = 0x00;
inline constexpr unsigned char small_uint_min = 0x01;
inline constexpr unsigned char small_uint_max = 0x3f;
inline constexpr unsigned char small_neg_min = 0x40;
inline constexpr unsigned char small_neg_max = 0x4f;
inline constexpr unsigned char int_payload_min = 0x50;
inline constexpr unsigned char int_payload_max = 0x5f;
inline constexpr unsigned char int_payload_sign = 0x08;
inline constexpr unsigned char bytes_base = 0x60;
inline constexpr unsigned char string_base = 0x70;
inline constexpr unsigned char array_base = 0x80;
inline constexpr unsigned char map_base = 0x90;
inline constexpr unsigned char object_base = 0xa0;
inline constexpr unsigned char sized_inline_max = 14;
inline constexpr unsigned char sized_extended = 15;
inline constexpr unsigned char bool_false_header = 0xb0;
inline constexpr unsigned char bool_true_header = 0xb1;
inline constexpr unsigned char null_header = 0xb2;
inline constexpr unsigned char float32_header = 0xc4;
inline constexpr unsigned char float64_header = 0xc8;

inline constexpr bool is_in_range(unsigned char value, unsigned char first, unsigned char last) noexcept {
    return value >= first && value <= last;
}

inline constexpr unsigned char size_base(value_kind kind) noexcept {
    switch (kind) {
    case value_kind::bytes:
        return bytes_base;
    case value_kind::string:
        return string_base;
    case value_kind::array:
        return array_base;
    case value_kind::map:
        return map_base;
    case value_kind::object:
        return object_base;
    default:
        return 0;
    }
}

inline constexpr bool is_sized_header(unsigned char header, value_kind kind) noexcept {
    const auto base = size_base(kind);
    return base != 0 && is_in_range(header, base, static_cast<unsigned char>(base + sized_extended));
}

inline constexpr std::uint8_t compact_byte_count(std::uint64_t value) noexcept {
    std::uint8_t bytes = 1;
    while ((value >>= 8u) != 0) {
        ++bytes;
    }
    return bytes;
}

using contract::adapters::base::is_byte_like_element_v;
using contract::adapters::base::trim_trailing_zeros;

// std::memcpy(dst, src, byte_count) with a runtime byte_count forces an
// out-of-line call: the compiler can't fold a variable-length copy into a
// single load. Dispatching on byte_count first turns each case into a
// fixed-size memcpy the compiler inlines as one (or two) load instructions.
inline std::uint64_t load_little_uint(const unsigned char* data, std::size_t byte_count) noexcept {
    std::uint64_t value = 0;
    switch (byte_count) {
    case 1: {
        std::uint8_t v;
        std::memcpy(&v, data, 1);
        value = v;
        break;
    }
    case 2: {
        std::uint16_t v;
        std::memcpy(&v, data, 2);
        value = v;
        break;
    }
    case 3: {
        std::uint32_t v = 0;
        std::memcpy(&v, data, 3);
        value = v;
        break;
    }
    case 4: {
        std::uint32_t v;
        std::memcpy(&v, data, 4);
        value = v;
        break;
    }
    case 5: {
        std::uint64_t v = 0;
        std::memcpy(&v, data, 5);
        value = v;
        break;
    }
    case 6: {
        std::uint64_t v = 0;
        std::memcpy(&v, data, 6);
        value = v;
        break;
    }
    case 7: {
        std::uint64_t v = 0;
        std::memcpy(&v, data, 7);
        value = v;
        break;
    }
    case 8:
        std::memcpy(&value, data, 8);
        break;
    default:
        break;
    }
    return value;
}

template<class Reader>
class skip_reader;

} // namespace detail

using detail::read_error;
using detail::read_error_code;
using detail::read_stage;
using detail::value_kind;
using detail::write_error;
using detail::write_error_code;
using detail::write_stage;

namespace attributes {

template<class Field>
inline std::optional<std::size_t> max_items_limit(const Field& descriptor) {
    if constexpr (base::has_field_context_v<Field>) {
        if constexpr (contract::has_attribute_v<Field, contract::check::max_items>) {
            const auto& max_items =
                descriptor.attributes.template get<contract::check::max_items>();
            return max_items.value;
        }
    }
    return std::nullopt;
}

} // namespace attributes

template<class Output = contract::io::window_output>
class writer {
public:
    using output_type = Output;

    static_assert(contract::io::has_window_output<Output>,
        "compact writer requires a window_output-like backend");

    explicit writer(Output output, options = {})
        : out_(std::move(output))
    {}

    writer with(options = {}) const {
        return writer{out_};
    }

    template<class O = Output, std::enable_if_t<std::is_same_v<O, contract::io::window_output>, int> = 0>
    explicit writer(unsigned char* data, std::size_t size, options = {})
        : out_(data, size)
    {}

    write_status write(const void* data, std::size_t size) {
        if (size == 0) {
            return write_status::ok;
        }
        if (data == nullptr) {
            return error()
                .code(write_error_code::invalid_size)
                .stage(write_stage::raw_bytes)
                .sizes(size, 0);
        }
        auto window = out_.prepare(size);
        if (window.size() != size) {
            return error()
                .code(write_error_code::output_error)
                .stage(write_stage::raw_bytes)
                .sizes(size, window.size());
        }
        std::memcpy(window.data(), data, size);
        out_.commit(size);
        return write_status::ok;
    }

    write_status write_byte(unsigned char value) {
        return write(&value, sizeof(value));
    }

    write_status write_uint(std::uint64_t value) {
        // zero_header (0x00) is itself the smallest small_uint header, so a
        // single range check covers both: writing `value` as the header byte
        // reproduces zero_header exactly when value == 0.
        if (value <= detail::small_uint_max) {
            return write_byte(static_cast<unsigned char>(value));
        }
        return write_integer_payload(false, value);
    }

    write_status write_int(std::int64_t value) {
        if (value >= 0) {
            return write_uint(static_cast<std::uint64_t>(value));
        }

        const auto magnitude = static_cast<std::uint64_t>(-(value + 1)) + 1u;
        if (magnitude <= 16u) {
            return write_byte(static_cast<unsigned char>(
                detail::small_neg_min + static_cast<unsigned char>(magnitude - 1u)));
        }
        return write_integer_payload(true, magnitude);
    }

    write_status write_bool(bool value) {
        return write_byte(value ? detail::bool_true_header : detail::bool_false_header);
    }

    write_status write_null() {
        return write_byte(detail::null_header);
    }

    write_status write_size_header(value_kind kind, std::uint64_t size) {
        const auto base = detail::size_base(kind);
        if (base == 0) {
            return error()
                .code(write_error_code::unsupported_operation)
                .stage(write_stage::header);
        }

        if (size <= detail::sized_inline_max) {
            return write_byte(static_cast<unsigned char>(base + static_cast<unsigned char>(size)));
        }
        if (write_byte(static_cast<unsigned char>(base + detail::sized_extended)) == write_status::error) {
            return write_status::error;
        }
        return write_uint(size);
    }

    write_status write_float(float value) {
        static_assert(std::numeric_limits<float>::is_iec559,
            "compact float32 requires IEEE-754 float");
        return write_little_uint(
            detail::float32_header,
            std::bit_cast<std::uint32_t>(value),
            sizeof(std::uint32_t));
    }

    write_status write_float(double value) {
        static_assert(std::numeric_limits<double>::is_iec559,
            "compact float64 requires IEEE-754 double");
        return write_little_uint(
            detail::float64_header,
            std::bit_cast<std::uint64_t>(value),
            sizeof(std::uint64_t));
    }

    template<class T>
    write_status write_value(const T& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
        return contract::adapters::compact::codec<value_type>::write(*this, value);
    }

    template<class T, std::enable_if_t<
        !contract::adapters::base::has_contract_definition<T>, int> = 0>
    writer& operator<<(const T& value) {
        clear_error();
        if (write_value(value) == write_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    const std::optional<write_error>& error() const noexcept {
        return error_;
    }

    [[nodiscard]]
    std::size_t position() const noexcept
        requires contract::io::has_position<Output>
    {
        return out_.position();
    }

    write_error& error(const write_error& child) noexcept {
        return error().transfer_from(child);
    }

    void clear_error() noexcept {
        error_.reset();
    }

    std::string error_message() const {
        return error_ ? error_->message() : write_error{}.message();
    }

private:
    template<class, class>
    friend struct codec;

    template<class Field, class Value>
    write_status write_field_value(const Field& descriptor, const Value& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<Value>>;

        using codec_type = contract::adapters::compact::codec<value_type>;
        if constexpr (contract::adapters::base::has_field_write<
                          codec_type, writer, Field, value_type>) {
            return codec_type::write(*this, descriptor, value);
        } else {
            return codec_type::write(*this, value);
        }
    }

    write_status write_integer_payload(bool negative, std::uint64_t magnitude) {
        const auto byte_count = detail::compact_byte_count(magnitude);
        const auto header = static_cast<unsigned char>(
            detail::int_payload_min |
            (negative ? detail::int_payload_sign : 0u) |
            static_cast<unsigned char>(byte_count - 1u));
        return write_little_uint(header, magnitude, byte_count);
    }

    write_status write_little_uint(std::uint64_t value, std::size_t byte_count) {
        auto window = out_.prepare(byte_count);
        if (window.size() != byte_count) {
            return error()
                .code(write_error_code::output_error)
                .stage(write_stage::raw_bytes)
                .sizes(byte_count, window.size());
        }

        auto* bytes = reinterpret_cast<unsigned char*>(window.data());
        if constexpr (std::endian::native == std::endian::little) {
            std::memcpy(bytes, &value, byte_count);
        } else {
            for (std::size_t i = 0; i < byte_count; ++i) {
                bytes[i] = static_cast<unsigned char>((value >> (i * 8u)) & 0xffu);
            }
        }
        out_.commit(byte_count);
        return write_status::ok;
    }

    write_status write_little_uint(unsigned char header, std::uint64_t value, std::size_t byte_count) {
        const auto total_size = byte_count + 1u;
        auto window = out_.prepare(total_size);
        if (window.size() != total_size) {
            const auto stage =
                header == detail::float32_header ? write_stage::float32 :
                header == detail::float64_header ? write_stage::float64 :
                                                   write_stage::raw_bytes;
            return error()
                .code(write_error_code::output_error)
                .stage(stage)
                .sizes(total_size, window.size());
        }

        auto* bytes = reinterpret_cast<unsigned char*>(window.data());
        bytes[0] = header;
        if constexpr (std::endian::native == std::endian::little) {
            std::memcpy(bytes + 1, &value, byte_count);
        } else {
            for (std::size_t i = 0; i < byte_count; ++i) {
                bytes[1 + i] = static_cast<unsigned char>((value >> (i * 8u)) & 0xffu);
            }
        }
        out_.commit(total_size);
        return write_status::ok;
    }

    write_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
            if constexpr (contract::io::has_position<Output>) {
                error_->offset(out_.position());
            }
        }
        return *error_;
    }

    output_type out_;
    std::optional<write_error> error_{};
};

template<class Input = contract::io::window_input>
class reader {
public:
    using input_type = Input;
    static_assert(contract::io::has_window_input<Input>,
        "compact reader requires a window_input-like backend");

    explicit reader(Input input, options = {})
        : in_(std::move(input))
    {}

    reader with(options = {}) const {
        return reader{in_};
    }

    template<class I = Input, std::enable_if_t<std::is_same_v<I, contract::io::window_input>, int> = 0>
    explicit reader(const unsigned char* data, std::size_t size, options = {})
        : in_(data, size)
    {}

    std::span<const std::byte> peek(std::size_t max_size) const noexcept {
        return in_.peek(max_size);
    }

    void consume(std::size_t size) noexcept {
        in_.consume(size);
    }

    read_status peek_byte(unsigned char& value) {
        const auto view = peek(1);
        if (view.size() != 1) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::header)
                .sizes(1, view.size());
        }
        value = static_cast<unsigned char>(view[0]);
        return read_status::ok;
    }

    void consume_byte() noexcept {
        consume(1);
    }

    read_status read(void* out, std::size_t size) {
        if (size == 0) {
            return read_status::ok;
        }
        if (out == nullptr) {
            return error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::raw_bytes)
                .sizes(size, 0);
        }
        const auto view = peek(size);
        if (view.size() != size) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::raw_bytes)
                .sizes(size, view.size());
        }
        std::memcpy(out, view.data(), size);
        consume(size);
        return read_status::ok;
    }

    read_status read_byte(unsigned char& value) {
        const auto view = peek(1);
        if (view.size() != 1) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::header)
                .sizes(1, view.size());
        }
        value = static_cast<unsigned char>(view[0]);
        consume_byte();
        return read_status::ok;
    }

    read_status read_uint(std::uint64_t& value) {
        const auto view = peek(9);
        if (view.empty()) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::header)
                .sizes(1, 0);
        }

        const auto header = static_cast<unsigned char>(view[0]);
        // zero_header (0x00) is itself the smallest small_uint header, so a
        // single range check covers both zero and small positive values.
        if (header <= detail::small_uint_max) {
            value = header;
            consume(1);
            return read_status::ok;
        }

        // int_payload_sign (0x08) is exactly the bit that splits the
        // int_payload range (0x50-0x5f) into unsigned (0x50-0x57) and signed
        // (0x58-0x5f) headers, so a single mask+compare identifies the only
        // other success case for read_uint without three separate checks.
        if ((header & 0xf8u) == detail::int_payload_min) {
            const auto byte_count = static_cast<std::size_t>((header & 0x07u) + 1u);
            const auto payload = view.subspan(1);
            if (payload.size() < byte_count) {
                return error()
                    .code(read_error_code::truncated)
                    .stage(read_stage::raw_bytes)
                    .sizes(byte_count, payload.size());
            }

            if constexpr (std::endian::native == std::endian::little) {
                value = detail::load_little_uint(
                    reinterpret_cast<const unsigned char*>(payload.data()), byte_count);
            } else {
                value = 0;
                const auto* data = reinterpret_cast<const unsigned char*>(payload.data());
                switch (byte_count) {
                case 8: value |= static_cast<std::uint64_t>(data[7]) << 56u; [[fallthrough]];
                case 7: value |= static_cast<std::uint64_t>(data[6]) << 48u; [[fallthrough]];
                case 6: value |= static_cast<std::uint64_t>(data[5]) << 40u; [[fallthrough]];
                case 5: value |= static_cast<std::uint64_t>(data[4]) << 32u; [[fallthrough]];
                case 4: value |= static_cast<std::uint64_t>(data[3]) << 24u; [[fallthrough]];
                case 3: value |= static_cast<std::uint64_t>(data[2]) << 16u; [[fallthrough]];
                case 2: value |= static_cast<std::uint64_t>(data[1]) << 8u; [[fallthrough]];
                case 1: value |= static_cast<std::uint64_t>(data[0]); break;
                default: break;
                }
            }
            consume(1 + byte_count);
            return read_status::ok;
        }

        // Error path (rare): reconstruct the original diagnostic distinction.
        if (header <= detail::small_neg_max) {
            return error()
                .code(read_error_code::invalid_integer)
                .stage(read_stage::integer);
        }
        if (header > detail::int_payload_max) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }
        // Remaining case: header in 0x58-0x5f, a signed int_payload header.
        return error()
            .code(read_error_code::invalid_integer)
            .stage(read_stage::integer);
    }

    read_status read_int(std::int64_t& value) {
        const auto view = peek(9);
        if (view.empty()) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::header)
                .sizes(1, 0);
        }

        const auto header = static_cast<unsigned char>(view[0]);
        // zero_header (0x00) is itself the smallest small_uint header, so a
        // single range check covers both zero and small positive values.
        if (header <= detail::small_uint_max) {
            value = header;
            consume(1);
            return read_status::ok;
        }
        if (header <= detail::small_neg_max) {
            value = -static_cast<std::int64_t>(header - detail::small_neg_min + 1u);
            consume(1);
            return read_status::ok;
        }
        if (header > detail::int_payload_max) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }

        std::uint64_t magnitude = 0;
        const auto byte_count = static_cast<std::size_t>((header & 0x07u) + 1u);
        const auto payload = view.subspan(1);
        if (payload.size() < byte_count) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::raw_bytes)
                .sizes(byte_count, payload.size());
        }

        if constexpr (std::endian::native == std::endian::little) {
            magnitude = detail::load_little_uint(
                reinterpret_cast<const unsigned char*>(payload.data()), byte_count);
        } else {
            const auto* data = reinterpret_cast<const unsigned char*>(payload.data());
            switch (byte_count) {
            case 8: magnitude |= static_cast<std::uint64_t>(data[7]) << 56u; [[fallthrough]];
            case 7: magnitude |= static_cast<std::uint64_t>(data[6]) << 48u; [[fallthrough]];
            case 6: magnitude |= static_cast<std::uint64_t>(data[5]) << 40u; [[fallthrough]];
            case 5: magnitude |= static_cast<std::uint64_t>(data[4]) << 32u; [[fallthrough]];
            case 4: magnitude |= static_cast<std::uint64_t>(data[3]) << 24u; [[fallthrough]];
            case 3: magnitude |= static_cast<std::uint64_t>(data[2]) << 16u; [[fallthrough]];
            case 2: magnitude |= static_cast<std::uint64_t>(data[1]) << 8u; [[fallthrough]];
            case 1: magnitude |= static_cast<std::uint64_t>(data[0]); break;
            default: break;
            }
        }

        const bool negative = (header & detail::int_payload_sign) != 0;
        if (!negative) {
            if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return error()
                    .code(read_error_code::invalid_integer)
                    .stage(read_stage::integer);
            }
            value = static_cast<std::int64_t>(magnitude);
            consume(1 + byte_count);
            return read_status::ok;
        }

        constexpr std::uint64_t min_magnitude =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1u;
        if (magnitude > min_magnitude) {
            return error()
                .code(read_error_code::invalid_integer)
                .stage(read_stage::integer);
        }
        if (magnitude == min_magnitude) {
            value = std::numeric_limits<std::int64_t>::min();
        } else {
            value = -static_cast<std::int64_t>(magnitude);
        }
        consume(1 + byte_count);
        return read_status::ok;
    }

    read_status read_bool(bool& value) {
        unsigned char header = 0;
        if (read_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (header == detail::bool_false_header) {
            value = false;
            return read_status::ok;
        }
        if (header == detail::bool_true_header) {
            value = true;
            return read_status::ok;
        }
        return error()
            .code(read_error_code::invalid_header)
            .stage(read_stage::header);
    }

    read_status read_null() {
        unsigned char header = 0;
        if (read_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (header != detail::null_header) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }
        return read_status::ok;
    }

    read_status read_size_header(value_kind expected, std::uint64_t& size) {
        unsigned char header = 0;
        if (read_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (!detail::is_sized_header(header, expected)) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }
        return read_size_tail(header, expected, size);
    }

    read_status read_float(float& value) {
        static_assert(std::numeric_limits<float>::is_iec559,
            "compact float32 requires IEEE-754 float");
        unsigned char header = 0;
        if (read_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (header != detail::float32_header) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }

        std::uint64_t bits = 0;
        if (read_little_uint<sizeof(std::uint32_t)>(bits) == read_status::error) {
            return error().stage(read_stage::float32);
        }
        value = std::bit_cast<float>(static_cast<std::uint32_t>(bits));
        return read_status::ok;
    }

    read_status read_float(double& value) {
        static_assert(std::numeric_limits<double>::is_iec559,
            "compact float64 requires IEEE-754 double");
        unsigned char header = 0;
        if (read_byte(header) == read_status::error) {
            return read_status::error;
        }
        if (header != detail::float64_header) {
            return error()
                .code(read_error_code::invalid_header)
                .stage(read_stage::header);
        }

        std::uint64_t bits = 0;
        if (read_little_uint<sizeof(std::uint64_t)>(bits) == read_status::error) {
            return error().stage(read_stage::float64);
        }
        value = std::bit_cast<double>(bits);
        return read_status::ok;
    }

    read_status skip_value() {
        return detail::skip_reader<reader>{*this}.read();
    }

    template<class T>
    read_status read_value(T& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
        return contract::adapters::compact::codec<value_type>::read(*this, value);
    }

    template<class T, std::enable_if_t<
        !contract::adapters::base::has_contract_definition<T>, int> = 0>
    reader& operator>>(T& value) {
        clear_error();
        if (read_value(value) == read_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    template<class T>
    reader& operator>>(const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "compact::reader cannot read into const storage");
        return *this;
    }

    const std::optional<read_error>& error() const noexcept {
        return error_;
    }

    read_error& error(const read_error& child) noexcept {
        return error().transfer_from(child);
    }

    void clear_error() noexcept {
        error_.reset();
    }

    std::string error_message() const {
        return error_ ? error_->message() : read_error{}.message();
    }

private:
    template<class, class>
    friend struct codec;

    template<class>
    friend class detail::skip_reader;

    template<class Field, class Value>
    read_status read_field_value(const Field& descriptor, Value& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<Value>>;

        using codec_type = contract::adapters::compact::codec<value_type>;
        if constexpr (contract::adapters::base::has_field_read<
                          codec_type, reader, Field, value_type>) {
            return codec_type::read(*this, descriptor, value);
        } else {
            return codec_type::read(*this, value);
        }
    }

    // ByteCount is a template parameter (not a runtime argument) so the
    // memcpy/shift below always operates on a compile-time-constant size,
    // regardless of whether this function itself gets inlined at its
    // (fixed-size, float32/float64) call sites.
    template<std::size_t ByteCount>
    read_status read_little_uint(std::uint64_t& value) {
        const auto view = peek(ByteCount);
        if (view.size() != ByteCount) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::raw_bytes)
                .sizes(ByteCount, view.size());
        }

        value = 0;
        if constexpr (std::endian::native == std::endian::little) {
            std::memcpy(&value, view.data(), ByteCount);
        } else {
            const auto* data = reinterpret_cast<const unsigned char*>(view.data());
            switch (ByteCount) {
            case 8: value |= static_cast<std::uint64_t>(data[7]) << 56u; [[fallthrough]];
            case 7: value |= static_cast<std::uint64_t>(data[6]) << 48u; [[fallthrough]];
            case 6: value |= static_cast<std::uint64_t>(data[5]) << 40u; [[fallthrough]];
            case 5: value |= static_cast<std::uint64_t>(data[4]) << 32u; [[fallthrough]];
            case 4: value |= static_cast<std::uint64_t>(data[3]) << 24u; [[fallthrough]];
            case 3: value |= static_cast<std::uint64_t>(data[2]) << 16u; [[fallthrough]];
            case 2: value |= static_cast<std::uint64_t>(data[1]) << 8u; [[fallthrough]];
            case 1: value |= static_cast<std::uint64_t>(data[0]); break;
            default: break;
            }
        }
        consume(ByteCount);
        return read_status::ok;
    }

    read_status read_size_tail(unsigned char header, value_kind expected, std::uint64_t& size) {
        const auto low = static_cast<unsigned char>(header - detail::size_base(expected));
        if (low < detail::sized_extended) {
            size = low;
            return read_status::ok;
        }
        return read_uint(size);
    }

    read_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
            if constexpr (contract::io::has_position<Input>) {
                error_->offset(in_.position());
            }
        }
        return *error_;
    }

    input_type in_;
    std::optional<read_error> error_{};
};

namespace detail {

template<class Reader>
class skip_reader {
public:
    explicit skip_reader(Reader& in) noexcept
        : in_(in) {}

    read_status read() {
        unsigned char header = 0;
        if (in_.read_byte(header) == read_status::error) {
            return read_status::error;
        }
        return read_from_header(header);
    }

private:
    read_status raw(std::uint64_t size) {
        unsigned char scratch[64]{};
        while (size != 0) {
            const auto chunk = static_cast<std::size_t>(
                size > sizeof(scratch) ? sizeof(scratch) : size);
            if (in_.read(scratch, chunk) == read_status::error) {
                return in_.error().stage(read_stage::skip);
            }
            size -= chunk;
        }
        return read_status::ok;
    }

    read_status size_tail(unsigned char header, value_kind expected, std::uint64_t& size) {
        const auto low = static_cast<unsigned char>(header - size_base(expected));
        if (low < sized_extended) {
            size = low;
            return read_status::ok;
        }
        return in_.read_uint(size);
    }

    read_status sized_payload(unsigned char header, value_kind kind) {
        std::uint64_t size = 0;
        if (size_tail(header, kind, size) == read_status::error) {
            return read_status::error;
        }
        return raw(size);
    }

    read_status repeated(unsigned char header, value_kind kind, std::uint8_t values_per_item) {
        std::uint64_t count = 0;
        if (size_tail(header, kind, count) == read_status::error) {
            return read_status::error;
        }
        for (std::uint64_t i = 0; i < count; ++i) {
            for (std::uint8_t value_index = 0; value_index < values_per_item; ++value_index) {
                if (read() == read_status::error) {
                    return read_status::error;
                }
            }
        }
        return read_status::ok;
    }

    read_status object(unsigned char header) {
        std::uint64_t field_count = 0;
        if (size_tail(header, value_kind::object, field_count) == read_status::error) {
            return read_status::error;
        }
        for (std::uint64_t i = 0; i < field_count; ++i) {
            std::uint64_t field_id = 0;
            if (in_.read_uint(field_id) == read_status::error) {
                return read_status::error;
            }
            if (read() == read_status::error) {
                return read_status::error;
            }
        }
        return read_status::ok;
    }

    read_status read_from_header(unsigned char header) {
        // Dispatch on the high nibble so the compiler builds a jump table
        // instead of a linear chain of range checks. The byte layout groups
        // each value_kind into one nibble (see docs/contract/adapters/compact.md):
        // 0x0..0x3 zero/small_uint, 0x4 small_neg, 0x5 int_payload, 0x6 bytes,
        // 0x7 string, 0x8 array, 0x9 map, 0xa object, 0xb bool/null, 0xc float.
        switch (header >> 4) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:  // zero / small_uint — self-contained, no payload
        case 0x4:  // small_neg — self-contained, no payload
            return read_status::ok;
        case 0x5:  // int_payload — low 3 bits hold byte_count-1
            return raw(static_cast<std::uint64_t>((header & 0x07u) + 1u));
        case 0x6:
            return sized_payload(header, value_kind::bytes);
        case 0x7:
            return sized_payload(header, value_kind::string);
        case 0x8:
            return repeated(header, value_kind::array, 1);
        case 0x9:
            return repeated(header, value_kind::map, 2);
        case 0xa:
            return object(header);
        case 0xb:  // bool_false / bool_true / null — exact values within nibble
            if (header == bool_false_header ||
                header == bool_true_header ||
                header == null_header) {
                return read_status::ok;
            }
            break;
        case 0xc:  // float32 / float64 — exact values within nibble
            if (header == float32_header) {
                return raw(sizeof(std::uint32_t));
            }
            if (header == float64_header) {
                return raw(sizeof(std::uint64_t));
            }
            break;
        default:
            break;
        }
        return in_.error()
            .code(read_error_code::invalid_header)
            .stage(read_stage::skip);
    }

    Reader& in_;
};

} // namespace detail

template<class T, class Enable>
struct codec {
    template<class Writer>
    static write_status write(Writer&, const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "compact::codec<T> is not defined for this contract value type");
        return write_status::error;
    }

    template<class Reader>
    static read_status read(Reader&, T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "compact::codec<T> is not defined for this contract value type");
        return read_status::error;
    }
};

template<class T>
struct codec<T, std::enable_if_t<contract::adapters::base::has_contract_definition<
    contract::adapters::base::clean_t<T>>, void>> {
    using object_type = contract::adapters::base::clean_t<T>;

    template<class Writer>
    static write_status write(Writer& out, const object_type& obj) {
        contract::require_adapter_mode<object_type, adapter_traits>();

        const auto type_name = contract::type_name<object_type>();
        if (out.write_size_header(value_kind::object, contract::field_count<object_type>()) ==
            write_status::error) {
            return out.error().type_name(type_name).stage(write_stage::object);
        }

        write_status status = write_status::ok;
        contract::for_each_field<object_type>(
            [&](const auto&... descriptors) {
                ((status == write_status::ok
                    ? status = write_field(out, descriptors, obj)
                    : status), ...);
            });
        if (status == write_status::error) {
            return out.error().type_name(type_name);
        }
        return write_status::ok;
    }

    template<class Reader>
    static read_status read(Reader& in, object_type& obj) {
        contract::require_adapter_mode<object_type, adapter_traits>();

        const auto type_name = contract::type_name<object_type>();
        std::uint64_t wire_field_count = 0;
        if (in.read_size_header(value_kind::object, wire_field_count) == read_status::error) {
            return in.error().type_name(type_name).stage(read_stage::object);
        }

        for (std::uint64_t i = 0; i < wire_field_count; ++i) {
            std::uint64_t field_id = 0;
            if (in.read_uint(field_id) == read_status::error) {
                return in.error().type_name(type_name).stage(read_stage::field_id);
            }
            if (field_id == 0) {
                return in.error()
                    .code(read_error_code::invalid_field_id)
                    .type_name(type_name)
                    .stage(read_stage::field_id);
            }

            bool matched = false;
            if (read_field_by_id<0>(in, obj, field_id, matched) == read_status::error) {
                return in.error().type_name(type_name);
            }
            if (!matched && in.skip_value() == read_status::error) {
                return in.error().type_name(type_name).stage(read_stage::skip);
            }
        }

        return read_status::ok;
    }

private:
    template<class Writer, class Field>
    static write_status write_field(Writer& out, const Field& descriptor, const object_type& obj) {
        if constexpr (Field::id <= 0) {
            return out.error()
                .code(write_error_code::invalid_field_id)
                .field(descriptor)
                .stage(write_stage::field_id);
        } else {
            if (out.write_uint(static_cast<std::uint64_t>(Field::id)) == write_status::error) {
                return out.error().field(descriptor).stage(write_stage::field_id);
            }

            auto&& value = descriptor.get(obj);
            if (out.write_field_value(descriptor, value) == write_status::error) {
                return out.error().field(descriptor).stage(write_stage::field_value);
            }
            return write_status::ok;
        }
    }

    template<class Reader, class Field>
    static read_status read_field(Reader& in, const Field& descriptor, object_type& obj) {
        using field_type = contract::adapters::base::clean_t<Field>;
        using value_type = typename field_type::value_type;

        if constexpr (field_type::template can_direct_ref<object_type>) {
            auto& value = descriptor.ref(obj);
            if (in.read_field_value(descriptor, value) == read_status::error) {
                return in.error().field(descriptor).stage(read_stage::field_value);
            }
            return read_status::ok;
        } else {
            static_assert(!std::is_array_v<value_type>,
                "compact::reader cannot load array fields through semantic fallback; "
                "use mutable physical storage or a custom codec");
            value_type value{};
            if (in.read_field_value(descriptor, value) == read_status::error) {
                return in.error().field(descriptor).stage(read_stage::field_value);
            }
            descriptor.set(obj, std::move(value));
            return read_status::ok;
        }
    }

    template<std::size_t Index, class Reader>
    static read_status read_field_by_id(
        Reader& in,
        object_type& obj,
        std::uint64_t field_id,
        bool& matched) {
        if constexpr (Index >= contract::field_count<object_type>()) {
            return read_status::ok;
        } else {
            auto field = contract::field_at<Index, object_type>();
            if (static_cast<std::uint64_t>(field.id) == field_id) {
                matched = true;
                return read_field(in, field, obj);
            }
            return read_field_by_id<Index + 1>(in, obj, field_id, matched);
        }
    }
};

template<>
struct codec<bool, void> {
    template<class Writer>
    static write_status write(Writer& out, bool value) {
        return out.write_bool(value);
    }

    template<class Reader>
    static read_status read(Reader& in, bool& value) {
        return in.read_bool(value);
    }
};

template<class T>
struct codec<T, std::enable_if_t<
    std::is_integral_v<contract::adapters::base::clean_t<T>> &&
    !std::is_same_v<contract::adapters::base::clean_t<T>, bool>, void>> {
    using value_type = contract::adapters::base::clean_t<T>;

    static_assert(sizeof(value_type) <= sizeof(std::uint64_t),
        "compact integral codec supports integer types up to 64 bits");

    template<class Writer>
    static write_status write(Writer& out, value_type value) {
        if constexpr (std::is_signed_v<value_type>) {
            return out.write_int(static_cast<std::int64_t>(value));
        } else {
            return out.write_uint(static_cast<std::uint64_t>(value));
        }
    }

    template<class Reader>
    static read_status read(Reader& in, value_type& value) {
        if constexpr (std::is_signed_v<value_type>) {
            std::int64_t decoded = 0;
            if (in.read_int(decoded) == read_status::error) {
                return read_status::error;
            }
            if (decoded < static_cast<std::int64_t>(std::numeric_limits<value_type>::min()) ||
                decoded > static_cast<std::int64_t>(std::numeric_limits<value_type>::max())) {
                return in.error()
                    .code(read_error_code::invalid_integer)
                    .stage(read_stage::integer);
            }
            value = static_cast<value_type>(decoded);
            return read_status::ok;
        } else {
            std::uint64_t decoded = 0;
            if (in.read_uint(decoded) == read_status::error) {
                return read_status::error;
            }
            if (decoded > static_cast<std::uint64_t>(std::numeric_limits<value_type>::max())) {
                return in.error()
                    .code(read_error_code::invalid_integer)
                    .stage(read_stage::integer);
            }
            value = static_cast<value_type>(decoded);
            return read_status::ok;
        }
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_enum_v<contract::adapters::base::clean_t<T>>, void>> {
    using value_type = contract::adapters::base::clean_t<T>;
    using underlying_type = std::underlying_type_t<value_type>;

    template<class Writer>
    static write_status write(Writer& out, value_type value) {
        return codec<underlying_type>::write(out, static_cast<underlying_type>(value));
    }

    template<class Reader>
    static read_status read(Reader& in, value_type& value) {
        underlying_type decoded{};
        if (codec<underlying_type>::read(in, decoded) == read_status::error) {
            return read_status::error;
        }
        value = static_cast<value_type>(decoded);
        return read_status::ok;
    }
};

template<>
struct codec<float, void> {
    template<class Writer>
    static write_status write(Writer& out, float value) {
        return out.write_float(value);
    }

    template<class Reader>
    static read_status read(Reader& in, float& value) {
        return in.read_float(value);
    }
};

template<>
struct codec<double, void> {
    template<class Writer>
    static write_status write(Writer& out, double value) {
        return out.write_float(value);
    }

    template<class Reader>
    static read_status read(Reader& in, double& value) {
        return in.read_float(value);
    }
};

template<>
struct codec<std::string, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::string& value) {
        if (out.write_size_header(value_kind::string, value.size()) == write_status::error) {
            return write_status::error;
        }
        if (value.empty()) {
            return write_status::ok;
        }
        return out.write(value.data(), value.size());
    }

    template<class Reader>
    static read_status read(Reader& in, std::string& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::string, size) == read_status::error) {
            return read_status::error;
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::size);
        }

        const auto byte_size = static_cast<std::size_t>(size);
        if (byte_size == 0) {
            value.clear();
            return read_status::ok;
        }
        value.resize(byte_size);
        return in.read(value.data(), byte_size);
    }
};

template<>
struct codec<std::string_view, void> {
    template<class Writer>
    static write_status write(Writer& out, std::string_view value) {
        if (out.write_size_header(value_kind::string, value.size()) == write_status::error) {
            return write_status::error;
        }
        if (value.empty()) {
            return write_status::ok;
        }
        return out.write(value.data(), value.size());
    }

    template<class Reader>
    static read_status read(Reader& in, std::string_view& value) {
        std::uint64_t size = 0;
        if (in.read_size_header(value_kind::string, size) == read_status::error) {
            return read_status::error;
        }
        if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return in.error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::size);
        }

        const auto byte_size = static_cast<std::size_t>(size);
        if (byte_size == 0) {
            value = std::string_view{};
            return read_status::ok;
        }
        const auto view = in.peek(byte_size);
        if (view.size() != byte_size) {
            return in.error()
                .code(read_error_code::truncated)
                .stage(read_stage::raw_bytes)
                .sizes(byte_size, view.size());
        }
        const auto* data = reinterpret_cast<const char*>(view.data());
        value = std::string_view{data, byte_size};
        in.consume(byte_size);
        return read_status::ok;
    }
};

template<class Object, class Output,
    std::enable_if_t<contract::adapters::base::has_contract_definition<Object>, int> = 0>
writer<Output>& operator<<(writer<Output>& out, const Object& obj) {
    out.clear_error();
    if (out.write_value(obj) == write_status::error) {
        throw std::runtime_error(out.error_message());
    }
    return out;
}

template<class Object, class Input,
    std::enable_if_t<contract::adapters::base::has_contract_definition<Object>, int> = 0>
reader<Input>& operator>>(reader<Input>& in, Object& obj) {
    in.clear_error();
    if (in.read_value(obj) == read_status::error) {
        throw std::runtime_error(in.error_message());
    }
    return in;
}

} // namespace contract::adapters::compact
