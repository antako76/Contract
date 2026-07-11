#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0
//
// Lean binary adapter core. Use <contract/adapters/binary/all.hpp> for the
// full standard-container family set.

#include <contract/adapters/base.hpp>
#include <contract/contract.hpp>
#include <contract/detail/error.hpp>
#include <contract/io/byte.hpp>
#include <contract/attributes/resolution.hpp>
#include <contract/attributes/validation.hpp>
#include <contract/check.hpp>
#include <contract/security.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <utility>

namespace contract::adapters {

namespace binary {

struct options {
    std::array<std::uint8_t, 4> encrypt_key{
        0x13u, 0x57u, 0x9Bu, 0xDFu
    };
};

struct adapter_traits {
    static constexpr contract::adapter_type type = contract::adapter_type::wire;
    static constexpr contract::attribute_visibility visibility =
        contract::attribute_visibility::declared_vocabularies_only;

    using visible_vocabularies = contract::vocabularies<
        contract::check::vocabulary,
        contract::security::vocabulary>;

    using attribute_rules = contract::attribute_rules<
        contract::default_for<contract::check::vocabulary>::ignore,
        contract::default_for<contract::security::vocabulary>::ignore,
        contract::for_tag<contract::check::tag::decode_guard>::enforce,
        contract::for_attr<contract::security::sensitive>::ignore,
        contract::for_attr<contract::security::secret>::ignore,
        contract::for_attr<contract::security::no_log>::out_of_scope,
        contract::for_attr<contract::security::encrypt>::enforce>;
};

// The second template parameter enables partial specializations for codec selection.
template<class T, class Enable = void>
struct codec;

using read_status = contract::adapters::base::status;
using write_status = contract::adapters::base::status;

namespace detail {

enum class read_error_code {
    unknown,
    input_error,
    truncated,
    invalid_size,
    max_length_exceeded,
    max_bytes_exceeded,
    max_items_exceeded,
    duplicate_key,
    variant_index_out_of_range,
    span_size_mismatch,
    unsupported_operation,
};

enum class write_error_code {
    unknown,
    output_error,
    invalid_size,
    max_length_exceeded,
    max_bytes_exceeded,
    max_items_exceeded,
    unsupported_operation,
};

enum class read_stage {
    none,
    raw,
    view,
    size,
    field_value,
    attribute_guard,
    container,
    variant,
    span,
};

enum class write_stage {
    none,
    raw,
    size,
    field_value,
    attribute_guard,
    container,
};

inline std::string_view to_string(read_error_code value) noexcept {
    switch (value) {
    case read_error_code::unknown:
        return "unknown";
    case read_error_code::input_error:
        return "input error";
    case read_error_code::truncated:
        return "truncated";
    case read_error_code::invalid_size:
        return "invalid size";
    case read_error_code::max_length_exceeded:
        return "max length exceeded";
    case read_error_code::max_bytes_exceeded:
        return "max bytes exceeded";
    case read_error_code::max_items_exceeded:
        return "max items exceeded";
    case read_error_code::duplicate_key:
        return "duplicate key";
    case read_error_code::variant_index_out_of_range:
        return "variant index out of range";
    case read_error_code::span_size_mismatch:
        return "span size mismatch";
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
    case write_error_code::max_length_exceeded:
        return "max length exceeded";
    case write_error_code::max_bytes_exceeded:
        return "max bytes exceeded";
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
    case read_stage::raw:
        return "raw bytes";
    case read_stage::view:
        return "read view";
    case read_stage::size:
        return "size";
    case read_stage::field_value:
        return "field value";
    case read_stage::attribute_guard:
        return "attribute guard";
    case read_stage::container:
        return "container";
    case read_stage::variant:
        return "variant";
    case read_stage::span:
        return "span";
    }
    return "input";
}

inline std::string_view to_string(write_stage value) noexcept {
    switch (value) {
    case write_stage::none:
        return "output";
    case write_stage::raw:
        return "raw bytes";
    case write_stage::size:
        return "size";
    case write_stage::field_value:
        return "field value";
    case write_stage::attribute_guard:
        return "attribute guard";
    case write_stage::container:
        return "container";
    }
    return "output";
}

struct read_error : contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status> {
public:
    using contract::detail::adapter_error_base<read_error, read_error_code, read_stage, read_status>::adapter_error_base;

    std::string message() const {
        return base_message("binary reader", "reading", read_stage::none);
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
        return base_message("binary writer", "writing", write_stage::none);
    }

private:
    friend class contract::detail::adapter_error_base<write_error, write_error_code, write_stage, write_status>;

    void append_adapter_details(std::string&) const {}
};

inline std::string to_string(const write_error& error) {
    return error.message();
}

template<class Output>
class encrypting_output {
public:
    explicit encrypting_output(Output& out, std::array<std::uint8_t, 4> key)
        : out_(&out)
        , key_(key) {}

    std::size_t write(const void* data, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (data == nullptr) {
            return 0;
        }

        scratch_.resize(size);
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            scratch_[i] = bytes[i] ^ key_[(offset_ + i) % key_.size()];
        }
        offset_ += size;
        return out_->write(scratch_.data(), size);
    }

    unsigned char* current() const {
        return out_->current();
    }

private:
    Output* out_;
    std::array<std::uint8_t, 4> key_;
    std::size_t offset_ = 0;
    std::vector<unsigned char> scratch_;
};

template<class Input>
class decrypting_input {
public:
    explicit decrypting_input(Input& in, std::array<std::uint8_t, 4> key)
        : in_(&in)
        , key_(key) {}

    const unsigned char* read_view(std::size_t size) {
        if (size == 0) {
            return in_->read_view(size);
        }

        const auto* bytes = in_->read_view(size);
        if (bytes == nullptr) {
            return nullptr;
        }
        scratch_.resize(size);
        for (std::size_t i = 0; i < size; ++i) {
            scratch_[i] = bytes[i] ^ key_[(offset_ + i) % key_.size()];
        }
        offset_ += size;
        return scratch_.data();
    }

    std::size_t read(void* out, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (out == nullptr) {
            return 0;
        }

        const auto* bytes = in_->read_view(size);
        if (bytes == nullptr) {
            return 0;
        }
        auto* dst = static_cast<std::uint8_t*>(out);
        for (std::size_t i = 0; i < size; ++i) {
            dst[i] = bytes[i] ^ key_[(offset_ + i) % key_.size()];
        }
        offset_ += size;
        return size;
    }

    const unsigned char* current() const {
        return in_->current();
    }

private:
    Input* in_;
    std::array<std::uint8_t, 4> key_;
    std::size_t offset_ = 0;
    mutable std::vector<unsigned char> scratch_;
};

} // namespace detail

using detail::read_error;
using detail::read_error_code;
using detail::read_stage;
using detail::write_error;
using detail::write_error_code;
using detail::write_stage;

namespace attributes {

template<class Field>
inline std::optional<std::size_t> max_length_limit(const Field& descriptor) {
    if constexpr (base::has_field_context_v<Field>) {
        if constexpr (contract::has_attribute_v<Field, contract::check::max_length>) {
            const auto& max_length =
                descriptor.attributes.template get<contract::check::max_length>();
            return max_length.value;
        }
    }
    return std::nullopt;
}

template<class Field>
inline std::optional<std::size_t> max_bytes_limit(const Field& descriptor) {
    if constexpr (base::has_field_context_v<Field>) {
        if constexpr (contract::has_attribute_v<Field, contract::check::max_bytes>) {
            const auto& max_bytes =
                descriptor.attributes.template get<contract::check::max_bytes>();
            return max_bytes.value;
        }
    }
    return std::nullopt;
}

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

namespace codec_detail {

// Raw byte codec applies only to scalar-like values. Composite/container
// types use dedicated codec specializations.
template<class T>
inline constexpr bool is_raw_codec_value_v =
    (std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, std::byte>) &&
    !std::is_pointer_v<T> &&
    !std::is_array_v<T> &&
    !std::is_same_v<T, std::string_view>;

template<class Reader>
concept has_status_read_view = requires(Reader& in, std::size_t size, const unsigned char*& data) {
    { in.read_view(size, data) } -> std::same_as<read_status>;
};

} // namespace codec_detail

// Field-access traits used by adapter traversal.
template<class Field, class Object>
inline constexpr bool can_direct_field_read_v = [] {
    using field_type = contract::adapters::base::clean_t<Field>;
    using object_type = std::remove_cvref_t<Object>;
    using value_type = typename field_type::value_type;
    using storage_type = typename field_type::storage_type;
    using normalized_storage_type = std::remove_cvref_t<storage_type>;
    return (field_type::is_member_field || field_type::is_reference_field) &&
           field_type::template can_direct_ref<Object> &&
           !field_type::template has_custom_set<Object&, value_type&&> &&
           !std::is_const_v<object_type> &&
           std::is_same_v<normalized_storage_type, value_type> &&
           !std::is_const_v<normalized_storage_type> &&
           !std::is_volatile_v<normalized_storage_type>;
}();

template<class T, class Enable>
struct codec {
    template<class Writer>
    static write_status write(Writer&, const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "binary::codec<T> is not defined for this contract value type");
        return write_status::error;
    }

    template<class Reader>
    static read_status read(Reader&, T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "binary::codec<T> is not defined for this contract value type");
        return read_status::error;
    }
};

template<class Output = contract::io::output>
class writer {
public:
    using output_type = Output;

    struct state {
        bool encrypt_active = false;
    };

    explicit writer(Output output, options opt = {}, state runtime_state = {})
        : out_(std::move(output))
        , opt_(opt)
        , state_(runtime_state) {}

    writer with(options opt = {}) const {
        return writer{out_, opt, state_};
    }

    template<class O = Output, std::enable_if_t<std::is_same_v<O, contract::io::output>, int> = 0>
    explicit writer(unsigned char* data, options opt = {}, state runtime_state = {})
        : out_(data)
        , opt_(opt)
        , state_(runtime_state) {}

    template<class O = Output, std::enable_if_t<std::is_same_v<O, contract::io::checked_output>, int> = 0>
    explicit writer(unsigned char* data, std::size_t size, options opt = {}, state runtime_state = {})
        : out_(data, size)
        , opt_(opt)
        , state_(runtime_state) {}

    write_status write(const void* data, std::size_t size) {
        if (size == 0) {
            return write_status::ok;
        }
        if (data == nullptr) {
            return error()
                .code(write_error_code::invalid_size)
                .stage(write_stage::raw)
                .sizes(size, 0);
        }

        if (out_.write(data, size) != size) {
            return error()
                .code(write_error_code::output_error)
                .stage(write_stage::raw)
                .sizes(size, 0);
        }
        return write_status::ok;
    }

    template<class T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
    write_status write(const T& value) {
        return write(&value, sizeof(value));
    }

    template<class T, std::enable_if_t<
        !contract::adapters::base::has_contract_definition<T> &&
        !std::is_array_v<std::remove_reference_t<T>>, int> = 0>
    writer& operator<<(const T& value) {
        error_.reset();
        if (write_value(value) == write_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    template<class T, std::size_t Size>
    writer& operator<<(T (&value)[Size]) {
        error_.reset();
        if (write_value(value) == write_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    template<class Field, class Object>
    write_status field(const Field& descriptor, const Object& obj) {
        auto&& value = descriptor.get(obj);

        if constexpr (contract::has_attribute_v<Field, contract::security::encrypt>) {
            if (!state_.encrypt_active) {
                detail::encrypting_output<output_type> encrypted_out{out_, opt_.encrypt_key};
                using encrypted_writer_type = writer<detail::encrypting_output<output_type>>;
                encrypted_writer_type encrypted_writer{
                    std::move(encrypted_out), opt_, typename encrypted_writer_type::state{true}};
                if (encrypted_writer.write_field_value(descriptor, value) == write_status::error) {
                    return error(encrypted_writer.error())
                        .field(descriptor)
                        .stage(write_stage::field_value);
                }
                return write_status::ok;
            }
        }

        if (write_field_value(descriptor, value) == write_status::error) {
            return error().field(descriptor).stage(write_stage::field_value);
        }
        return write_status::ok;
    }

    template<class T>
    write_status write_value(const T& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
        return contract::adapters::binary::codec<value_type>::write(*this, value);
    }

    template<class T>
    write_status write_value(const T& value)
        requires contract::adapters::base::has_contract_definition<std::remove_cv_t<std::remove_reference_t<T>>> {
        return write_object(value);
    }

    unsigned char* current() const {
        return out_.current();
    }

    const std::optional<write_error>& error() const noexcept {
        return error_;
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
    template<class>
    friend class writer;

    template<class, class>
    friend struct codec;

    template<class Field, class Value>
    write_status write_field_value(const Field& descriptor, const Value& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<Value>>;

        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            return write_object(value);
        } else {
            using codec_type = contract::adapters::binary::codec<value_type>;
            if constexpr (contract::adapters::base::has_field_write<codec_type, writer, Field, value_type>) {
                return codec_type::write(*this, descriptor, value);
            } else {
                return codec_type::write(*this, value);
            }
        }
    }

    template<class Object>
    write_status write_object(const Object& obj) {
        contract::require_adapter_mode<Object, adapter_traits>();
        write_status status = write_status::ok;
        contract::for_each_field<Object>(
            [&](const auto&... descriptors) {
                ((status == write_status::ok
                    ? status = field(descriptors, obj)
                    : status), ...);
            });
        return status;
    }

    write_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
        }
        return *error_;
    }

    output_type out_;
    options opt_;
    state state_{};
    std::optional<write_error> error_{};
};

template<class Input = contract::io::input>
class reader {
public:
    using input_type = Input;

    struct state {
        bool encrypt_active = false;
    };

    explicit reader(Input input, options opt = {}, state runtime_state = {})
        : in_(std::move(input))
        , opt_(opt)
        , state_(runtime_state) {}

    reader with(options opt = {}) const {
        return reader{in_, opt, state_};
    }

    template<class I = Input, std::enable_if_t<std::is_same_v<I, contract::io::input>, int> = 0>
    explicit reader(const unsigned char* data, options opt = {}, state runtime_state = {})
        : in_(data)
        , opt_(opt)
        , state_(runtime_state) {}

    template<class I = Input, std::enable_if_t<std::is_same_v<I, contract::io::checked_input>, int> = 0>
    explicit reader(const unsigned char* data, std::size_t size, options opt = {}, state runtime_state = {})
        : in_(data, size)
        , opt_(opt)
        , state_(runtime_state) {}

    template<class I = Input, std::enable_if_t<contract::io::has_read_view<I>, int> = 0>
    read_status read_view(std::size_t size, const unsigned char*& data) {
        data = nullptr;
        data = in_.read_view(size);
        if (data == nullptr && size != 0) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::view)
                .sizes(size, 0);
        }
        return read_status::ok;
    }

    read_status read(void* out, std::size_t size) {
        if (size == 0) {
            return read_status::ok;
        }
        if (out == nullptr) {
            return error()
                .code(read_error_code::invalid_size)
                .stage(read_stage::raw)
                .sizes(size, 0);
        }

        if (in_.read(out, size) != size) {
            return error()
                .code(read_error_code::truncated)
                .stage(read_stage::raw)
                .sizes(size, 0);
        }
        return read_status::ok;
    }

    template<class T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
    read_status read(T& value) {
        return read(&value, sizeof(value));
    }

    template<class T, std::enable_if_t<
        !contract::adapters::base::has_contract_definition<T> &&
        !std::is_array_v<std::remove_reference_t<T>>, int> = 0>
    reader& operator>>(T& value) {
        error_.reset();
        if (read_value(value) == read_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    template<class T, std::size_t Size>
    reader& operator>>(T (&value)[Size]) {
        error_.reset();
        if (read_value(value) == read_status::error) {
            throw std::runtime_error(error_message());
        }
        return *this;
    }

    template<class T>
    reader& operator>>(const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "binary::reader cannot read into const storage");
        return *this;
    }

    template<class Field, class Object>
    read_status field(const Field& descriptor, Object& obj) {
        using field_type = contract::adapters::base::clean_t<Field>;
        using value_type = typename field_type::value_type;

        if constexpr (binary::can_direct_field_read_v<Field, Object>) {
            auto& value = descriptor.ref(obj);
            if constexpr (contract::has_attribute_v<Field, contract::security::encrypt>) {
                if (!state_.encrypt_active) {
                    detail::decrypting_input<input_type> encrypted_in{in_, opt_.encrypt_key};
                    using encrypted_reader_type = reader<detail::decrypting_input<input_type>>;
                    encrypted_reader_type encrypted_reader{
                        std::move(encrypted_in), opt_, typename encrypted_reader_type::state{true}};
                    if (encrypted_reader.read_field_value(descriptor, value) == read_status::error) {
                        return error(encrypted_reader.error())
                            .field(descriptor)
                            .stage(read_stage::field_value);
                    }
                    return read_status::ok;
                }
            }
            if (read_field_value(descriptor, value) == read_status::error) {
                return error().field(descriptor).stage(read_stage::field_value);
            }
            return read_status::ok;
        } else {
            static_assert(!std::is_array_v<value_type>,
                "binary::reader cannot load array fields through semantic fallback; "
                "use mutable physical storage or a custom codec");
            value_type value{};
            if constexpr (contract::has_attribute_v<Field, contract::security::encrypt>) {
                if (!state_.encrypt_active) {
                    detail::decrypting_input<input_type> encrypted_in{in_, opt_.encrypt_key};
                    using encrypted_reader_type = reader<detail::decrypting_input<input_type>>;
                    encrypted_reader_type encrypted_reader{
                        std::move(encrypted_in), opt_, typename encrypted_reader_type::state{true}};
                    if (encrypted_reader.read_field_value(descriptor, value) == read_status::error) {
                        return error(encrypted_reader.error())
                            .field(descriptor)
                            .stage(read_stage::field_value);
                    }
                } else {
                    if (read_field_value(descriptor, value) == read_status::error) {
                        return error().field(descriptor).stage(read_stage::field_value);
                    }
                }
            } else {
                if (read_field_value(descriptor, value) == read_status::error) {
                    return error().field(descriptor).stage(read_stage::field_value);
                }
            }
            descriptor.set(obj, std::move(value));
            return read_status::ok;
        }
    }

    template<class T>
    read_status read_value(T& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
        return contract::adapters::binary::codec<value_type>::read(*this, value);
    }

    template<class T>
    read_status read_value(T& value)
        requires contract::adapters::base::has_contract_definition<std::remove_cv_t<std::remove_reference_t<T>>> {
        return read_object(value);
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
    template<class>
    friend class reader;

    template<class, class>
    friend struct codec;

    template<class Field, class Value>
    read_status read_field_value(const Field& descriptor, Value& value) {
        using value_type = std::remove_cv_t<std::remove_reference_t<Value>>;

        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            return read_object(value);
        } else {
            using codec_type = contract::adapters::binary::codec<value_type>;
            if constexpr (contract::adapters::base::has_field_read<codec_type, reader, Field, value_type>) {
                return codec_type::read(*this, descriptor, value);
            } else {
                return codec_type::read(*this, value);
            }
        }
    }

    template<class Object>
    read_status read_object(Object& obj) {
        contract::require_adapter_mode<Object, adapter_traits>();
        read_status status = read_status::ok;
        contract::for_each_field<Object>(
            [&](const auto&... descriptors) {
                ((status == read_status::ok
                    ? status = field(descriptors, obj)
                    : status), ...);
            });
        return status;
    }

    read_error& error(std::source_location location = std::source_location::current()) noexcept {
        if (!error_) {
            error_.emplace(location);
        }
        return *error_;
    }

    input_type in_;
    options opt_;
    state state_{};
    std::optional<read_error> error_{};
};

// Value codecs are the typed layer: operator<</>> dispatches here, and codecs
// call writer/reader raw primitives when the on-wire format is just bytes.
template<class T>
struct codec<T, std::enable_if_t<codec_detail::is_raw_codec_value_v<T>, void>> {
    template<class Writer>
    static write_status write(Writer& out, const T& value) {
        return out.write(value);
    }

    template<class Reader>
    static read_status read(Reader& in, T& value) {
        return in.read(value);
    }
};

template<>
struct codec<std::string, void> {
    template<class Writer>
    static write_status write(Writer& out, const std::string& value) {
        const std::size_t size = value.size();
        if (out.write_value(size) == write_status::error) {
            return write_status::error;
        }
        if (value.empty()) {
            return write_status::ok;
        }
        return out.write(value.data(), size);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::string& value) {
        if (const auto limit = attributes::max_length_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_length_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_bytes_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::string& value) {
        std::size_t size = 0;
        if (in.read_value(size) == read_status::error) {
            return read_status::error;
        }
        if (size == 0) {
            value.clear();
            return read_status::ok;
        }
        value.resize(size);
        return in.read(value.data(), size);
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::string& value) {
        std::size_t size = 0;
        if (in.read_value(size) == read_status::error) {
            return read_status::error;
        }
        if (const auto limit = attributes::max_length_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_length_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_bytes_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if (size != 0) {
            value.resize(size);
            return in.read(value.data(), size);
        } else {
            value.clear();
        }
        return read_status::ok;
    }
};

template<>
struct codec<std::string_view, void> {
    template<class Writer>
    static write_status write(Writer& out, std::string_view value) {
        const std::size_t size = value.size();
        if (out.write_value(size) == write_status::error) {
            return write_status::error;
        }
        if (value.empty()) {
            return write_status::ok;
        }
        return out.write(value.data(), size);
    }

    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, std::string_view value) {
        if (const auto limit = attributes::max_length_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_length_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_bytes_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        return write(out, value);
    }

    template<class Reader>
    static read_status read(Reader& in, std::string_view& value) {
        static_assert(codec_detail::has_status_read_view<Reader>,
            "binary::codec<std::string_view> requires contiguous input with read_view(size, data)");

        std::size_t size = 0;
        if (in.read_value(size) == read_status::error) {
            return read_status::error;
        }

        if (size == 0) {
            value = {};
            return read_status::ok;
        }

        const unsigned char* data = nullptr;
        if (in.read_view(size, data) == read_status::error) {
            return read_status::error;
        }
        value = std::string_view{reinterpret_cast<const char*>(data), size};
        return read_status::ok;
    }

    template<class Reader, class Field>
    static read_status read(Reader& in, const Field& field, std::string_view& value) {
        static_assert(codec_detail::has_status_read_view<Reader>,
            "binary::codec<std::string_view> requires contiguous input with read_view(size, data)");

        std::size_t size = 0;
        if (in.read_value(size) == read_status::error) {
            return read_status::error;
        }
        if (const auto limit = attributes::max_length_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_length_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && size > *limit) {
            return in.error()
                .code(read_error_code::max_bytes_exceeded)
                .field(field)
                .stage(read_stage::attribute_guard)
                .sizes(*limit, size);
        }

        if (size == 0) {
            value = {};
            return read_status::ok;
        }

        const unsigned char* data = nullptr;
        if (in.read_view(size, data) == read_status::error) {
            return read_status::error;
        }
        value = std::string_view{reinterpret_cast<const char*>(data), size};
        return read_status::ok;
    }
};

template<>
struct codec<const char*, void> {
    // C-string fields are write-only and use [size_t len][bytes][zero], unlike
    // std::string's [size_t len][bytes].
    template<class Writer>
    static write_status write(Writer& out, const char* value) {
        const char* data = value == nullptr ? "" : value;
        const std::size_t size = std::char_traits<char>::length(data);
        if (out.write_value(size) == write_status::error) {
            return write_status::error;
        }
        if (size > 0) {
            if (out.write(data, size) == write_status::error) {
                return write_status::error;
            }
        }
        const char zero = '\0';
        return out.write(&zero, 1);
    }

    template<class Reader, class Field>
    static read_status read(Reader&, const Field&, const char*&) {
        static_assert(contract::adapters::base::always_false_v<Reader>,
            "binary::codec<const char*> read is disabled; use owned string storage");
        return read_status::error;
    }
};

// Contract-object traversal hooks.
template<class Object, class Output, std::enable_if_t<contract::adapters::base::has_contract_definition<Object>, int> = 0>
writer<Output>& operator<<(writer<Output>& out, const Object& obj) {
    out.clear_error();
    if (out.write_value(obj) == write_status::error) {
        throw std::runtime_error(out.error_message());
    }
    return out;
}

template<class Object, class Input, std::enable_if_t<contract::adapters::base::has_contract_definition<Object>, int> = 0>
reader<Input>& operator>>(reader<Input>& in, Object& obj) {
    in.clear_error();
    if (in.read_value(obj) == read_status::error) {
        throw std::runtime_error(in.error_message());
    }
    return in;
}

} // namespace binary

} // namespace contract::adapters
