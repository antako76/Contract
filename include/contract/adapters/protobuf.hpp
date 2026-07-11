#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/contract.hpp>
#include <contract/detail/error.hpp>
#include <contract/io/byte.hpp>
#include <contract/io/byte_window.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace contract::adapters::protobuf {

struct options {
    bool pack_repeated_scalars = true;
};

template<class T, class Enable = void>
struct codec;

using read_status = contract::adapters::base::status;
using write_status = contract::adapters::base::status;

namespace detail {

enum class wire_type : std::uint8_t {
    varint = 0,
    fixed64 = 1,
    length_delimited = 2,
    fixed32 = 5,
};

enum class read_stage {
    none,
    message_root,
    field_key,
    field_value,
    varint,
    fixed32,
    fixed64,
    length,
};

enum class write_stage {
    none,
    message_root,
    field_value,
    tag,
    length,
    varint,
    fixed32,
    fixed64,
};

enum class read_error_code {
    unknown,
    truncated,
    invalid_varint,
    field_number_zero,
    unknown_field,
    duplicate_field,
    wire_type_mismatch,
    invalid_size,
    nested_error,
};

enum class write_error_code {
    unknown,
    output_error,
};

inline std::string_view to_string(read_error_code value) noexcept {
    switch (value) {
    case read_error_code::unknown:
        return "unknown";
    case read_error_code::truncated:
        return "truncated";
    case read_error_code::invalid_varint:
        return "invalid varint";
    case read_error_code::field_number_zero:
        return "field number zero";
    case read_error_code::unknown_field:
        return "unknown field";
    case read_error_code::duplicate_field:
        return "duplicate field";
    case read_error_code::wire_type_mismatch:
        return "wire type mismatch";
    case read_error_code::invalid_size:
        return "invalid size";
    case read_error_code::nested_error:
        return "nested error";
    }
    return "unknown";
}

inline std::string_view to_string(write_error_code value) noexcept {
    switch (value) {
    case write_error_code::unknown:
        return "unknown";
    case write_error_code::output_error:
        return "output error";
    }
    return "unknown";
}

inline std::string_view to_string(wire_type value) noexcept {
    switch (value) {
    case wire_type::varint:
        return "varint";
    case wire_type::fixed64:
        return "fixed64";
    case wire_type::length_delimited:
        return "length_delimited";
    case wire_type::fixed32:
        return "fixed32";
    }
    return "unknown";
}

inline std::string_view to_string(read_stage value) noexcept {
    switch (value) {
    case read_stage::none:
        return "parse";
    case read_stage::message_root:
        return "message";
    case read_stage::field_key:
        return "field key";
    case read_stage::field_value:
        return "field value";
    case read_stage::varint:
        return "varint";
    case read_stage::fixed32:
        return "fixed32";
    case read_stage::fixed64:
        return "fixed64";
    case read_stage::length:
        return "length";
    }
    return "parse";
}

inline std::string_view to_string(write_stage value) noexcept {
    switch (value) {
    case write_stage::none:
        return "output";
    case write_stage::message_root:
        return "message";
    case write_stage::field_value:
        return "field value";
    case write_stage::tag:
        return "tag";
    case write_stage::length:
        return "length";
    case write_stage::varint:
        return "varint";
    case write_stage::fixed32:
        return "fixed32";
    case write_stage::fixed64:
        return "fixed64";
    }
    return "output";
}

struct read_error : contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status> {
public:
    using contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status>::adapter_error_base;

    read_error& field_number(std::uint32_t value) noexcept {
        if (!wire_field_number_) {
            wire_field_number_ = value;
        }
        return *this;
    }

    read_error& wire(wire_type value) noexcept {
        wire_ = value;
        return *this;
    }

    read_error& expected_wire(wire_type value) noexcept {
        if (!expected_wire_) {
            expected_wire_ = value;
        }
        return *this;
    }

    std::string message() const {
        return base_message("protobuf reader", "reading", read_stage::none);
    }

private:
    friend class contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status>;

    void append_adapter_details(std::string& message) const {
        if (wire_field_number_) {
            message += " (wire field #";
            message += std::to_string(*wire_field_number_);
            message += ")";
        }

        if (expected_wire_ && wire_) {
            message += " (wire type mismatch: expected ";
            message += to_string(*expected_wire_);
            message += ", got ";
            message += to_string(*wire_);
            message += ")";
        }
    }

    std::optional<std::uint32_t> wire_field_number_{};
    std::optional<wire_type> wire_{};
    std::optional<wire_type> expected_wire_{};
};

inline std::string to_string(const read_error& error) {
    return error.message();
}

struct write_error : contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status> {
public:
    using contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status>::adapter_error_base;

    write_error& field_number(std::uint32_t value) noexcept {
        if (!wire_field_number_) {
            wire_field_number_ = value;
        }
        return *this;
    }

    write_error& wire(wire_type value) noexcept {
        if (!wire_) {
            wire_ = value;
        }
        return *this;
    }

    std::string message() const {
        return base_message("protobuf writer", "writing", write_stage::none);
    }

private:
    friend class contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status>;

    void append_adapter_details(std::string& message) const {
        if (wire_field_number_) {
            message += " (wire field #";
            message += std::to_string(*wire_field_number_);
            message += ")";
        }

        if (wire_) {
            message += " (wire type ";
            message += to_string(*wire_);
            message += ")";
        }
    }

    std::optional<std::uint32_t> wire_field_number_{};
    std::optional<wire_type> wire_{};
};

inline std::string to_string(const write_error& error) {
    return error.message();
}

using contract::adapters::base::is_byte_like_element_v;
using contract::adapters::base::trim_trailing_zeros;

// Number of bytes a varint encoding of value takes up, without encoding it.
// byte_count = ceil(bit_width(value) / 7), value|1 folds the value==0 case
// (which needs 1 byte) into the same formula as value==1.
constexpr std::size_t varint_byte_count(std::uint64_t value) noexcept {
    const unsigned bits = 64u - static_cast<unsigned>(std::countl_zero(value | 1u));
    return (bits + 6u) / 7u;
}

} // namespace detail

using detail::read_error;
using detail::read_error_code;
using detail::write_error;
using detail::write_error_code;

template<class Input = contract::io::window_input>
class reader {
public:
    using input_type = Input;
    static_assert(contract::io::has_window_input<Input>,
        "protobuf reader requires a window_input-like backend");

    explicit reader(Input input, options opt = {})
        : in_(std::move(input))
        , opt_(opt) {}

    reader with(options opt = {}) const {
        reader copy{*this};
        copy.opt_ = opt;
        return copy;
    }

    std::string error_message() const {
        if (!error_) {
            return "protobuf reader: no error";
        }
        return error_->message();
    }

    const std::optional<detail::read_error>& error() const noexcept {
        return error_;
    }

    detail::read_error& error(const detail::read_error& value) noexcept {
        return error().transfer_from(value);
    }

    detail::read_error& error(const detail::read_error& value, std::size_t nested_size) noexcept {
        auto& result = error(value);
        if constexpr (contract::io::has_position<Input>) {
            result.position_correction(position() - nested_size);
        }
        return result;
    }

    [[nodiscard]]
    std::size_t position() const noexcept
        requires contract::io::has_position<Input>
    {
        return in_.position();
    }

    template<class T>
    read_status read_message(T& value) {
        static_assert(contract::adapters::base::has_contract_definition<T>,
            "protobuf reader can only read CONTRACT messages as message roots");

        error_.reset();
        const auto type_name = contract::type_name<std::remove_reference_t<T>>();

        while (!in_.peek(1).empty()) {
            std::uint32_t field_number = 0;
            detail::wire_type wire{};
            const auto key_status = read_tag(field_number, wire);
            if (key_status == read_status::error) {
                return error().type_name(type_name).stage(detail::read_stage::field_key);
            }

            if (field_number == 0) {
                return error().code(read_error_code::field_number_zero)
                    .type_name(type_name)
                    .stage(detail::read_stage::field_key)
                    .wire(wire);
            }

            const auto field_status = read_field_by_number<std::remove_reference_t<decltype(value)>, 0>(
                *this, value, field_number, wire);
            if (field_status == read_status::error) {
                return field_status;
            }
        }
        return read_status::ok;
    }

    read_status read_varint(std::uint64_t& value) {
        value = 0;
        const auto window = in_.peek(10);
        if (window.empty()) {
            return error().code(read_error_code::truncated).sizes(10, 0);
        }

        int shift = 0;
        for (std::size_t i = 0; i < window.size() && i < 10; ++i) {
            const unsigned char byte = static_cast<unsigned char>(window[i]);
            value |= static_cast<std::uint64_t>(byte & 0x7fu) << shift;
            if ((byte & 0x80u) == 0) {
                in_.consume(i + 1);
                return read_status::ok;
            }
            shift += 7;
        }

        if (window.size() < 10) {
            return error().code(read_error_code::truncated).sizes(10, window.size());
        }

        return error().code(read_error_code::invalid_varint).sizes(10, 10);
    }

    static std::pair<std::uint32_t, detail::wire_type> parse_tag(std::uint64_t key) noexcept {
        return {
            static_cast<std::uint32_t>(key >> 3),
            static_cast<detail::wire_type>(key & 0x7u),
        };
    }

    read_status read_tag(std::uint32_t& field_number, detail::wire_type& wire) {
        std::uint64_t key = 0;
        if (read_varint(key) == read_status::error) {
            return read_status::error;
        }

        const auto tag = parse_tag(key);
        field_number = tag.first;
        wire = tag.second;
        return read_status::ok;
    }

    read_status read_fixed32(std::uint32_t& value) {
        const unsigned char* data = nullptr;
        const auto status = read_view(sizeof(value), data);
        if (status == read_status::error) {
            return status;
        }

        std::memcpy(&value, data, sizeof(value));
        return read_status::ok;
    }

    read_status read_fixed64(std::uint64_t& value) {
        const unsigned char* data = nullptr;
        const auto status = read_view(sizeof(value), data);
        if (status == read_status::error) {
            return status;
        }

        std::memcpy(&value, data, sizeof(value));
        return read_status::ok;
    }

    read_status read_view(std::size_t size, const unsigned char*& data) {
        if (size == 0) {
            data = nullptr;
            return read_status::ok;
        }

        const auto window = in_.peek(size);
        if (window.size() != size) {
            return error().code(read_error_code::truncated).sizes(size, window.size());
        }

        in_.consume(size);
        data = reinterpret_cast<const unsigned char*>(window.data());
        return read_status::ok;
    }

    [[nodiscard]]
    bool has_remaining() const noexcept
        requires contract::io::has_window_input<Input>
    {
        return !in_.peek(1).empty();
    }

private:
    template<class, class>
    friend struct codec;

    detail::read_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
            if constexpr (contract::io::has_position<Input>) {
                error_->offset(in_.position());
            }
        }
        return *error_;
    }

    template<class Object, std::size_t Index>
    static read_status read_field_by_number(
        reader& in,
        Object& obj,
        std::uint32_t field_number,
        detail::wire_type wire)
    {
        if constexpr (Index >= contract::field_count<Object>()) {
            const auto type_name = contract::type_name<Object>();
            in.error().code(read_error_code::unknown_field)
                .type_name(type_name)
                .field_number(field_number)
                .stage(detail::read_stage::field_key)
                .wire(wire);
            return read_status::error;
        } else {
            auto field = contract::field_at<Index, Object>();
            if (static_cast<std::uint32_t>(field.id) == field_number) {
                return in.read_field(field, obj, wire);
            }
            return read_field_by_number<Object, Index + 1>(in, obj, field_number, wire);
        }
    }

    template<class Field, class Object>
    read_status read_field(const Field& field, Object& obj, detail::wire_type wire) {
        using field_type = contract::adapters::base::clean_t<Field>;
        using value_type = typename field_type::value_type;
        using codec_type = codec<value_type>;

        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            if constexpr (field_type::template can_direct_ref<Object>) {
                auto& slot = field.ref(obj);
                return codec_type::read(*this, field, wire, slot);
            } else {
                value_type slot{};
                const auto status = codec_type::read(*this, field, wire, slot);
                if (status == read_status::error) {
                    error().field(field).stage(detail::read_stage::field_value);
                    return status;
                }
                field.set(obj, std::move(slot));
                return status;
            }
        } else if constexpr (field_type::template can_direct_ref<Object>) {
            auto& slot = field.ref(obj);
            return codec_type::read(*this, field, wire, slot);
        } else {
            value_type slot{};
            const auto status = codec_type::read(*this, field, wire, slot);
            if (status == read_status::error) {
                error().field(field).stage(detail::read_stage::field_value);
                return status;
            }
            field.set(obj, std::move(slot));
            return status;
        }
    }

    Input in_;
    options opt_{};
    std::optional<detail::read_error> error_{};
};

struct counting_output;

template<class Output = contract::io::window_output>
class writer {
public:
    using output_type = Output;
    static_assert(contract::io::has_write<Output> || contract::io::has_window_output<Output>,
        "protobuf writer requires a write() or window_output-like backend");

    explicit writer(Output output, options opt = {})
        : out_(output)
        , opt_(opt) {}

    writer with(options opt = {}) const {
        return writer{out_, opt};
    }

    std::string error_message() const {
        if (!error_) {
            return "protobuf writer: no error";
        }
        return error_->message();
    }

    const std::optional<detail::write_error>& error() const noexcept {
        return error_;
    }

    write_status write(const void* data, std::size_t size) {
        return write_bytes(data, size);
    }

    [[nodiscard]]
    std::size_t position() const noexcept
        requires contract::io::has_position<Output>
    {
        return out_.position();
    }

    write_status write_tag(std::uint32_t field_number, detail::wire_type type) {
        const std::uint64_t key =
            (static_cast<std::uint64_t>(field_number) << 3) |
            static_cast<std::uint64_t>(type);
        const auto status = write_varint_payload(key);
        if (status == write_status::error) {
            return error().stage(detail::write_stage::tag)
                .field_number(field_number)
                .wire(type);
        }
        return status;
    }

    write_status write_varint(std::uint64_t value) {
        const auto status = write_varint_payload(value);
        if (status == write_status::error) {
            return error().stage(detail::write_stage::varint);
        }
        return status;
    }

    write_status write_fixed32(std::uint32_t value) {
        unsigned char bytes[4];
        bytes[0] = static_cast<unsigned char>(value & 0xffu);
        bytes[1] = static_cast<unsigned char>((value >> 8) & 0xffu);
        bytes[2] = static_cast<unsigned char>((value >> 16) & 0xffu);
        bytes[3] = static_cast<unsigned char>((value >> 24) & 0xffu);
        const auto status = write_bytes(bytes, sizeof(bytes));
        if (status == write_status::error) {
            return error().stage(detail::write_stage::fixed32);
        }
        return status;
    }

    write_status write_fixed64(std::uint64_t value) {
        unsigned char bytes[8];
        bytes[0] = static_cast<unsigned char>(value & 0xffu);
        bytes[1] = static_cast<unsigned char>((value >> 8) & 0xffu);
        bytes[2] = static_cast<unsigned char>((value >> 16) & 0xffu);
        bytes[3] = static_cast<unsigned char>((value >> 24) & 0xffu);
        bytes[4] = static_cast<unsigned char>((value >> 32) & 0xffu);
        bytes[5] = static_cast<unsigned char>((value >> 40) & 0xffu);
        bytes[6] = static_cast<unsigned char>((value >> 48) & 0xffu);
        bytes[7] = static_cast<unsigned char>((value >> 56) & 0xffu);
        const auto status = write_bytes(bytes, sizeof(bytes));
        if (status == write_status::error) {
            return error().stage(detail::write_stage::fixed64);
        }
        return status;
    }

    template<class T>
    writer& operator<<(const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        error_.reset();
        const auto status = write_value(value);
        if (status == write_status::error) {
            if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
                error().type_name(contract::type_name<value_type>())
                    .stage(detail::write_stage::message_root);
            }
            throw std::runtime_error(error_message());
        }
        if constexpr (!contract::adapters::base::has_contract_definition<value_type>) {
            static_assert(contract::adapters::base::always_false_v<value_type>,
                "protobuf root values must be CONTRACT messages");
        }
        return *this;
    }

    template<class Field, class Object>
    write_status field(const Field& field, const Object& obj) {
        auto&& value = field.get(obj);
        return write_field(field, value);
    }

private:
    template<class, class>
    friend struct codec;
    template<class Value>
    friend std::optional<std::size_t> measure_encoded_size(const Value& value);

    detail::write_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
            if constexpr (contract::io::has_position<Output>) {
                error_->offset(out_.position());
            }
        }
        return *error_;
    }

    write_status write_bytes(const void* data, std::size_t size) {
        if (size == 0) {
            return write_status::ok;
        }
        if (data == nullptr) {
            return error().code(write_error_code::output_error).sizes(size, 0);
        }

        if constexpr (contract::io::has_write<Output>) {
            try {
                out_.write(data, size);
                return write_status::ok;
            } catch (...) {
                return error().code(write_error_code::output_error).sizes(size, 0);
            }
        } else {
            const auto* bytes = static_cast<const std::byte*>(data);
            std::size_t written = 0;
            while (written != size) {
                auto window = out_.prepare(size - written);
                if (window.empty()) {
                    return error().code(write_error_code::output_error).sizes(size, written);
                }

                std::memcpy(window.data(), bytes + written, window.size());
                out_.commit(window.size());
                written += window.size();
            }
            return write_status::ok;
        }
    }

    write_status write_varint_payload(std::uint64_t value) {
        // counting_output only wants the byte count, not the actual bytes -
        // skip encoding entirely instead of building bytes just to discard them.
        if constexpr (std::is_same_v<std::remove_reference_t<Output>, counting_output>) {
            out_.write(nullptr, detail::varint_byte_count(value));
            return write_status::ok;
        } else if constexpr (contract::io::has_window_output<Output>) {
            // Write straight into the window instead of a stack buffer + memcpy
            // (runtime-sized memcpy defeats inlining). 10 bytes always fits any
            // 64-bit varint.
            auto window = out_.prepare(10);
            if (window.size() >= 10) {
                std::size_t count = 0;
                std::uint64_t v = value;
                while (v >= 0x80u) {
                    window[count] = static_cast<std::byte>((v & 0x7fu) | 0x80u);
                    ++count;
                    v >>= 7;
                }
                window[count] = static_cast<std::byte>(v);
                ++count;
                out_.commit(count);
                return write_status::ok;
            }
        }
        return write_varint_payload_fallback(value);
    }

    // Forced noinline: this is the rare boundary-crossing path. Left to the
    // compiler, its single call site gets it inlined back into
    // write_varint_payload, which then grows too large to inline itself at
    // its many call sites in a caller with lots of scalar fields.
    [[gnu::noinline]] write_status write_varint_payload_fallback(std::uint64_t value) {
        unsigned char bytes[10];
        std::size_t count = 0;

        while (value >= 0x80u) {
            bytes[count++] = static_cast<unsigned char>((value & 0x7fu) | 0x80u);
            value >>= 7;
        }
        bytes[count++] = static_cast<unsigned char>(value);
        return write_bytes(bytes, count);
    }

    template<class Field, class Value>
    write_status write_field(const Field& field, const Value& value) {
        using value_type = contract::adapters::base::clean_t<Value>;

        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            const auto status = codec<value_type>::write(*this, field, value);
            if (status == write_status::error) {
                return error().field(field).stage(detail::write_stage::field_value);
            }
            return status;
        } else {
            using codec_type = codec<value_type>;
            if constexpr (contract::adapters::base::has_field_write<codec_type, writer, Field, value_type>) {
                const auto status = codec_type::write(*this, field, value);
                if (status == write_status::error) {
                    return error().field(field).stage(detail::write_stage::field_value);
                }
                return status;
            } else {
                static_assert(contract::adapters::base::always_false_v<value_type>,
                    "protobuf fields require a field-aware codec overload");
            }
        }
    }

    template<class T>
    write_status write_value(const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            return write_message_by_index<std::remove_cvref_t<T>, 0>(value);
        } else {
            static_assert(contract::adapters::base::always_false_v<value_type>,
                "protobuf root values must be CONTRACT messages");
            return error().code(write_error_code::unknown).stage(detail::write_stage::message_root);
        }
    }

    template<class Object, std::size_t Index>
    write_status write_message_by_index(const Object& obj) {
        using object_type = std::remove_cvref_t<Object>;
        if constexpr (Index >= contract::field_count<object_type>()) {
            return write_status::ok;
        } else {
            auto descriptor = contract::field_at<Index, object_type>();
            const auto status = this->field(descriptor, obj);
            if (status == write_status::error) {
                return status;
            }
            return write_message_by_index<object_type, Index + 1>(obj);
        }
    }

    Output out_;
    options opt_{};
    std::optional<detail::write_error> error_{};
};

struct counting_output {
    void write(const void*, std::size_t size) noexcept {
        position_ += size;
    }

    void reset() noexcept {
        position_ = 0;
    }

    [[nodiscard]]
    std::size_t position() const noexcept {
        return position_;
    }

private:
    std::size_t position_ = 0;
};

template<class Value>
std::optional<std::size_t> measure_encoded_size(const Value& value) {
    counting_output sizing{};
    writer<counting_output&> sizing_writer{sizing};
    using value_type = contract::adapters::base::clean_t<Value>;
    const auto status = codec<value_type>::write(sizing_writer, value);
    if (status == write_status::error) {
        return std::nullopt;
    }
    return sizing_writer.position();
}

template<class T, class Enable>
struct codec {
    template<class Writer>
    static write_status write(Writer&, const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "protobuf::codec<T> is not defined for this contract value type");
        return write_status::error;
    }
};

template<class T>
struct codec<T, std::enable_if_t<contract::adapters::base::has_contract_definition<T>, void>> {
    template<class Writer>
    static write_status write(Writer& out, const T& value) {
        return out.write_value(value);
    }

    template<class Reader>
    static read_status read(Reader& in, T& value) {
        // Critical boundary: the caller must pass a reader already limited to one embedded message.
        return in.read_message(value);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T& value) {
        const auto size = measure_encoded_size(value);
        if (!size) {
            return out.error().code(write_error_code::output_error)
                .field(field)
                .stage(detail::write_stage::length);
        }

        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            return write_status::error;
        }
        if (out.write_varint(*size) == write_status::error) {
            return write_status::error;
        }
        return codec<T>::write(out, value);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, T& value) {
        if (wire != detail::wire_type::length_delimited) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
        }

        std::uint64_t length = 0;
        const auto length_status = in.read_varint(length);
        if (length_status == read_status::error) {
            return in.error()
                .field(field)
                .wire(wire);
        }

        const unsigned char* data = nullptr;
        const auto view_status = in.read_view(static_cast<std::size_t>(length), data);
        if (view_status == read_status::error) {
            return in.error()
                .field(field)
                .wire(wire);
        }

        reader<> nested{contract::io::window_input{data, static_cast<std::size_t>(length)}, in.opt_};
        const auto status = codec<T>::read(nested, value);
        if (status == read_status::error) {
            return in.error(nested.error(), static_cast<std::size_t>(length))
                .field(field)
                .wire(wire);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, bool>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, bool value) {
        return out.write_varint(value ? 1u : 0u);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, bool value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::varint) == write_status::error) {
            return write_status::error;
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, bool& value) {
        std::uint64_t raw = 0;
        const auto status = in.read_varint(raw);
        if (status == read_status::error) {
            return status;
        }
        value = raw != 0u;
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, bool& value) {
        if (wire != detail::wire_type::varint) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::varint);
        }
        const auto status = read(in, value);
        if (status == read_status::error) {
            return in.error()
                .field(field)
                .wire(wire);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<
    std::is_integral_v<contract::adapters::base::clean_t<T>> &&
    !std::is_same_v<contract::adapters::base::clean_t<T>, bool> &&
    !std::is_same_v<contract::adapters::base::clean_t<T>, char> &&
    !std::is_same_v<contract::adapters::base::clean_t<T>, signed char> &&
    !std::is_same_v<contract::adapters::base::clean_t<T>, unsigned char>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, const T& value) {
        return out.write_varint(static_cast<std::uint64_t>(value));
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T& value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::varint) == write_status::error) {
            return write_status::error;
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, T& value) {
        std::uint64_t raw = 0;
        const auto status = in.read_varint(raw);
        if (status == read_status::error) {
            return status;
        }
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_signed_v<value_type>) {
            value = static_cast<value_type>(static_cast<std::int64_t>(raw));
        } else {
            value = static_cast<value_type>(raw);
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, T& value) {
        if (wire != detail::wire_type::varint) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::varint);
        }
        const auto status = read(in, value);
        if (status == read_status::error) {
            in.error()
                .field(field)
                .wire(wire);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_floating_point_v<contract::adapters::base::clean_t<T>>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_same_v<value_type, float>) {
            std::uint32_t bits;
            std::memcpy(&bits, &value, sizeof(bits));
            return out.write_fixed32(bits);
        } else {
            static_assert(std::is_same_v<value_type, double>, "protobuf floating point codec supports float and double only");
            std::uint64_t bits;
            std::memcpy(&bits, &value, sizeof(bits));
            return out.write_fixed64(bits);
        }
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_same_v<value_type, float>) {
            if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::fixed32) == write_status::error) {
                return write_status::error;
            }
            return write(out, value);
        } else {
            if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::fixed64) == write_status::error) {
                return write_status::error;
            }
            return write(out, value);
        }
    }

    template<class Reader>
    static read_status read(Reader& in, T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_same_v<value_type, float>) {
            std::uint32_t bits = 0;
            const auto status = in.read_fixed32(bits);
            if (status == read_status::error) {
                return status;
            }
            std::memcpy(&value, &bits, sizeof(bits));
        } else {
            static_assert(std::is_same_v<value_type, double>, "protobuf floating point codec supports float and double only");
            std::uint64_t bits = 0;
            const auto status = in.read_fixed64(bits);
            if (status == read_status::error) {
                return status;
            }
            std::memcpy(&value, &bits, sizeof(bits));
        }
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_same_v<value_type, float>) {
            if (wire != detail::wire_type::fixed32) {
                return in.error().code(read_error_code::wire_type_mismatch)
                    .field(field)
                    .wire(wire)
                    .expected_wire(detail::wire_type::fixed32);
            }
            const auto status = read(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
            }
            return status;
        } else {
            static_assert(std::is_same_v<value_type, double>, "protobuf floating point codec supports float and double only");
            if (wire != detail::wire_type::fixed64) {
                return in.error().code(read_error_code::wire_type_mismatch)
                    .field(field)
                    .wire(wire)
                    .expected_wire(detail::wire_type::fixed64);
            }
            const auto status = read(in, value);
            if (status == read_status::error) {
                in.error().field(field).wire(wire);
            }
            return status;
        }
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_enum_v<contract::adapters::base::clean_t<T>>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, const T& value) {
        using underlying_type = std::underlying_type_t<contract::adapters::base::clean_t<T>>;
        return codec<underlying_type>::write(out, static_cast<underlying_type>(value));
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const T& value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::varint) == write_status::error) {
            return write_status::error;
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, T& value) {
        using underlying_type = std::underlying_type_t<contract::adapters::base::clean_t<T>>;
        underlying_type raw{};
        const auto status = codec<underlying_type>::read(in, raw);
        if (status == read_status::error) {
            return status;
        }
        value = static_cast<T>(raw);
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, T& value) {
        if (wire != detail::wire_type::varint) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::varint);
        }
        const auto status = read(in, value);
        if (status == read_status::error) {
            return in.error().field(field).wire(wire);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, std::string>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, const std::string& value) {
        if (out.write_varint(value.size()) == write_status::error) {
            return write_status::error;
        }
        if (!value.empty()) {
            return out.write(value.data(), value.size());
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::string& value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            return write_status::error;
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::string& value) {
        std::uint64_t size = 0;
        const auto length_status = in.read_varint(size);
        if (length_status == read_status::error) {
            return in.error().stage(detail::read_stage::length);
        }
        const unsigned char* data = nullptr;
        const auto view_status = in.read_view(static_cast<std::size_t>(size), data);
        if (view_status == read_status::error) {
            return in.error().stage(detail::read_stage::field_value);
        }
        value.assign(reinterpret_cast<const char*>(data), static_cast<std::size_t>(size));
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::string& value) {
        if (wire != detail::wire_type::length_delimited) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
        }
        const auto status = read(in, value);
        if (status == read_status::error) {
            return in.error().field(field).wire(wire);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, std::string_view>, void>> {
public:
    template<class Writer>
    static write_status write(Writer& out, std::string_view value) {
        if (out.write_varint(value.size()) == write_status::error) {
            return write_status::error;
        }
        if (!value.empty()) {
            return out.write(value.data(), value.size());
        }
        return write_status::ok;
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, std::string_view value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            return write_status::error;
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::string_view& value) {
        std::uint64_t size = 0;
        const auto length_status = in.read_varint(size);
        if (length_status == read_status::error) {
            return in.error().stage(detail::read_stage::length);
        }
        const unsigned char* data = nullptr;
        const auto view_status = in.read_view(static_cast<std::size_t>(size), data);
        if (view_status == read_status::error) {
            return in.error().stage(detail::read_stage::field_value);
        }
        value = std::string_view{reinterpret_cast<const char*>(data), static_cast<std::size_t>(size)};
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, detail::wire_type wire, std::string_view& value) {
        if (wire != detail::wire_type::length_delimited) {
            return in.error().code(read_error_code::wire_type_mismatch)
                .field(field)
                .wire(wire)
                .expected_wire(detail::wire_type::length_delimited);
        }
        const auto status = read(in, value);
        if (status == read_status::error) {
            return in.error().field(field).wire(wire).stage(detail::read_stage::length);
        }
        return status;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, const char*>, void>> {
public:
    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const char* value) {
        if (out.write_tag(static_cast<std::uint32_t>(field.id), detail::wire_type::length_delimited) == write_status::error) {
            return write_status::error;
        }
        if (value == nullptr) {
            return out.write_varint(0);
        }

        const std::size_t size = std::char_traits<char>::length(value);
        if (out.write_varint(size) == write_status::error) {
            return write_status::error;
        }
        if (size != 0) {
            return out.write(value, size);
        }
        return write_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader&, const Field&, detail::wire_type, const char*&) {
        static_assert(contract::adapters::base::always_false_v<Reader>,
            "protobuf reader does not support const char* storage");
        return read_status::error;
    }
};

template<class Object, class Input, std::enable_if_t<contract::adapters::base::has_contract_definition<Object>, int> = 0>
reader<Input>& operator>>(reader<Input>& in, Object& obj) {
    const auto status = in.read_message(obj);
    if (status == read_status::error) {
        throw std::runtime_error(in.error_message());
    }
    return in;
}

} // namespace contract::adapters::protobuf
