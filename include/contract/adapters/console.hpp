#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/adapters/debug/field_comment.hpp>
#include <contract/adapters/debug/format.hpp>
#include <contract/adapters/debug/type_name.hpp>
#include <contract/contract.hpp>
#include <contract/security.hpp>

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <tuple>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace contract::adapters::console {

struct options;

namespace detail {

struct measurement_sink {
    std::size_t current_line_width = 0;
    std::vector<std::size_t> comment_widths;

    measurement_sink& operator<<(std::string_view value) {
        consume(value);
        return *this;
    }

    measurement_sink& operator<<(const std::string& value) {
        consume(value);
        return *this;
    }

    measurement_sink& operator<<(const char* value) {
        if (value == nullptr) {
            consume("(null)");
        } else {
            consume(std::string_view(value));
        }

        return *this;
    }

    measurement_sink& operator<<(char value) {
        if (value == '\n') {
            finish_line();
        } else {
            ++current_line_width;
        }

        return *this;
    }

    template<class T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
    measurement_sink& operator<<(T value) {
        if constexpr (std::is_enum_v<T>) {
            using underlying_type = std::underlying_type_t<T>;
            consume(std::to_string(static_cast<underlying_type>(value)));
        } else {
            consume(std::to_string(value));
        }
        return *this;
    }

    void mark_comment() {
        comment_widths.push_back(current_line_width);
    }

private:
    void finish_line() {
        current_line_width = 0;
    }

    void consume(std::string_view value) {
        for (char ch : value) {
            if (ch == '\n') {
                finish_line();
            } else {
                ++current_line_width;
            }
        }
    }
};

struct render_state {
    std::vector<std::size_t> comment_paddings;
    std::size_t comment_cursor = 0;

    void reset() {
        comment_paddings.clear();
        comment_cursor = 0;
    }

    std::size_t next_comment_padding() {
        if (comment_cursor < comment_paddings.size()) {
            return comment_paddings[comment_cursor++];
        }

        return 1;
    }
};

enum class text_color {
    field_name,
    type_name,
    string_literal,
    comment_field_id,
    comment_provenance,
    comment_attribute,
    comment_security,
    comment,
    truncation,
    number
};

template<text_color Color>
struct palette_for;

template<>
struct palette_for<text_color::field_name> {
    static constexpr auto member = &contract::adapters::debug::color_palette::field_name;
};

template<>
struct palette_for<text_color::type_name> {
    static constexpr auto member = &contract::adapters::debug::color_palette::type_name;
};

template<>
struct palette_for<text_color::string_literal> {
    static constexpr auto member = &contract::adapters::debug::color_palette::string_value;
};

template<>
struct palette_for<text_color::comment_field_id> {
    static constexpr auto member = &contract::adapters::debug::color_palette::comment_field_id;
};

template<>
struct palette_for<text_color::comment_provenance> {
    static constexpr auto member = &contract::adapters::debug::color_palette::comment_provenance;
};

template<>
struct palette_for<text_color::comment_attribute> {
    static constexpr auto member = &contract::adapters::debug::color_palette::comment_attribute;
};

template<>
struct palette_for<text_color::comment_security> {
    static constexpr auto member = &contract::adapters::debug::color_palette::comment_security;
};

template<>
struct palette_for<text_color::comment> {
    static constexpr auto member = &contract::adapters::debug::color_palette::comment;
};

template<>
struct palette_for<text_color::truncation> {
    static constexpr auto member = &contract::adapters::debug::color_palette::truncation;
};

template<>
struct palette_for<text_color::number> {
    static constexpr auto member = &contract::adapters::debug::color_palette::number_value;
};

struct node_kind {
    struct contract_object_t {};
    struct sequence_t {};
    struct string_t {};
    struct scalar_t {};

    static constexpr contract_object_t contract_object{};
    static constexpr sequence_t sequence{};
    static constexpr string_t string{};
    static constexpr scalar_t scalar{};
};

inline constexpr node_kind::contract_object_t node_kind::contract_object;
inline constexpr node_kind::sequence_t node_kind::sequence;
inline constexpr node_kind::string_t node_kind::string;
inline constexpr node_kind::scalar_t node_kind::scalar;

} // namespace detail

struct options {
    enum class mode {
        schema,
        value
    };

    mode output_mode = mode::schema;

    contract::adapters::debug::field_comment_options field_comment{};
    contract::adapters::debug::color_options color{};
    bool show_container_size = true;
    bool show_indexes = true;

    bool deterministic = true;
    bool align_comments = false;

    std::size_t indent = 2;
    std::size_t max_depth = 8;
    std::size_t max_items = 32;
    std::size_t max_string_length = 64;
    std::size_t max_byte_preview_length = 16;
};

template<class Output = std::ostream&>
class writer;

// The second template parameter enables partial specializations for codec selection.
template<class T, class Enable = void>
struct codec {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer&, const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "console::codec<T> is not defined for this contract value type");
    }

};

namespace codec_detail {

template<class T>
using clean_t = contract::adapters::base::clean_t<T>;

template<class T>
inline constexpr bool is_scalar_value_v =
    (std::is_arithmetic_v<T> || std::is_enum_v<T>) &&
    !std::is_same_v<T, bool>;

template<class T>
inline constexpr bool is_byte_blob_v =
    std::is_same_v<T, std::byte> ||
    std::is_same_v<T, unsigned char> ||
    std::is_same_v<T, char> ||
    std::is_same_v<T, signed char>;

// SFINAE dispatch for codec-specific comment metadata.
template<class Codec, class Writer, class T>
auto write_comment_if_present(Writer& out, const T& value, int)
    -> decltype(Codec::write_comment(out, value), void()) {
    Codec::write_comment(out, value);
}

template<class Codec, class Writer, class T>
void write_comment_if_present(Writer&, const T&, long) {}

template<class Codec, class Writer, class T>
void write_comment_if_present(Writer& out, const T& value) {
    write_comment_if_present<Codec>(out, value, 0);
}

template<class Codec, class Writer, class T, class = void>
struct has_write_comment : std::false_type {};

template<class Codec, class Writer, class T>
struct has_write_comment<Codec, Writer, T,
    std::void_t<decltype(Codec::write_comment(std::declval<Writer&>(), std::declval<const T&>()))>>
    : std::true_type {};

template<class Codec, class Writer, class T>
inline constexpr bool has_write_comment_v = has_write_comment<Codec, Writer, T>::value;

} // namespace codec_detail

template<class Output>
class writer {
public:
    using output_type = Output;
    using text_color = detail::text_color;

    explicit writer(Output output, options opt = {})
        : out_(output)
        , opt_(opt) {}

    writer schema() const {
        auto opt = opt_;
        opt.output_mode = options::mode::schema;
        opt.color.enabled = true;
        opt.align_comments = true;
        return writer{out_, std::move(opt)};
    }

    writer debug() const {
        auto opt = opt_;
        opt.output_mode = options::mode::schema;
        opt.color.enabled = true;
        opt.align_comments = true;
        opt.field_comment.show_base_offset = true;
        return writer{out_, std::move(opt)};
    }

    writer value() const {
        auto opt = opt_;
        opt.output_mode = options::mode::value;
        opt.align_comments = false;
        return writer{out_, std::move(opt)};
    }

    writer with(options opt) const {
        return writer{out_, std::move(opt)};
    }

    template<class T>
    writer& operator<<(const T& value) {
        render(value);
        return *this;
    }

public:
    writer& write(std::string_view value) {
        out_ << value;
        return *this;
    }

    writer& write(const std::string& value) {
        out_ << value;
        return *this;
    }

    writer& write(const char* value) {
        if (value == nullptr) {
            return write("(null)");
        }

        return write(std::string_view(value));
    }

    writer& write(char value) {
        out_ << '\'';

        switch (value) {
        case '\'':
            out_ << std::string_view("\\'");
            break;
        case '\\':
            out_ << std::string_view("\\\\");
            break;
        case '\n':
            out_ << std::string_view("\\n");
            break;
        case '\r':
            out_ << std::string_view("\\r");
            break;
        case '\t':
            out_ << std::string_view("\\t");
            break;
        default:
            out_ << value;
            break;
        }

        out_ << '\'';
        return *this;
    }

    template<class T>
    void render(const T& value) {
        state_.reset();

        if (needs_comment_layout()) {
            prepare_comment_layout(value);
        }

        write_value(value);
    }

public:
    template<class T>
    void write_value(const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        console::codec<value_type>::write(*this, value);
    }

    template<class T>
    void write(const T& value, detail::node_kind::contract_object_t) {
        write_contract_object(value);
    }

    template<class T>
    void write(const T& value, detail::node_kind::sequence_t) {
        write_sequence(value);
    }

    template<class T>
    void write(const T& value, detail::node_kind::string_t) {
        write_string(value);
    }

    template<class T>
    void write(const T& value, detail::node_kind::scalar_t) {
        write_scalar(value);
    }

private:
    template<class Field, class Object>
    void field(const Field& descriptor, const Object& obj) {
        using field_type = contract::adapters::base::clean_t<Field>;
        using attributes_type = typename field_type::attributes_type;
        using value_type = typename field_type::value_type;
        constexpr bool redacted =
            attributes_type::template contains<contract::security::secret>() ||
            attributes_type::template contains<contract::security::no_log>() ||
            attributes_type::template contains<contract::security::sensitive>();

        indent();
        write<text_color::field_name>(descriptor.name);
        out_ << ':';
        decltype(auto) value = descriptor.get(obj);

        if constexpr (redacted) {
            out_ << ' ';
            write<text_color::comment_security>("<redacted>");
            write_field_comment<Field, Object>(descriptor, value);
            newline();
        } else {
            if constexpr (console::codec<value_type>::block) {
                write_field_comment<Field, Object>(descriptor, value);
                newline();
                scoped_depth guard(*this);
                write_value(value);
            } else {
                out_ << ' ';
                write_value(value);
                write_field_comment<Field, Object>(descriptor, value);
                newline();
            }
        }
    }

    template<class T>
    void write_contract_object_fields(const T& value) {
        using object_type = contract::adapters::base::clean_t<T>;

        scoped_depth guard(*this);
        contract::for_each_field<object_type>(
            [&](const auto&... descriptors) {
                (field(descriptors, value), ...);
            });
    }

    template<class T>
    void write_contract_object(const T& value) {
        using object_type = contract::adapters::base::clean_t<T>;

        if (depth_ >= opt_.max_depth) {
            indent();
            write<text_color::truncation>("...");
            write<text_color::comment>(" # max depth reached");
            newline();
            return;
        }

        indent();
        write<text_color::type_name>(contract::type_name<object_type>());
        out_ << ':';
        newline();

        write_contract_object_fields(value);
    }

    template<class Field, class Object, class Value>
    void write_field_comment(const Field& descriptor, const Value& value) {
        if (opt_.output_mode != options::mode::schema) {
            return;
        }

        using field_type = contract::adapters::base::clean_t<Field>;
        using object_type = contract::adapters::base::clean_t<Object>;
        using value_type = contract::adapters::base::clean_t<Value>;
        using attributes_type = typename field_type::attributes_type;
        constexpr bool redacted =
            attributes_type::template contains<contract::security::secret>() ||
            attributes_type::template contains<contract::security::no_log>() ||
            attributes_type::template contains<contract::security::sensitive>();
        bool started = false;

        if (opt_.field_comment.show_field_ids) {
            begin_or_separate_comment(started);
            write<text_color::comment_field_id>("#");
            write<text_color::comment_field_id>(field_type::id);
        }

        if (opt_.field_comment.show_field_types) {
            if (opt_.field_comment.show_field_ids) {
                write<text_color::comment>(" ");
            } else {
                begin_or_separate_comment(started);
            }

            if constexpr (!std::is_void_v<typename field_type::storage_type> &&
                          !std::is_same_v<typename field_type::storage_type, typename field_type::value_type>) {
                if (opt_.field_comment.show_storage_type_when_different) {
                    write<text_color::comment>("value=");
                }
            }

            write<text_color::comment>(contract::adapters::debug::type_name<typename field_type::value_type>());
        }

        if constexpr (!std::is_void_v<typename field_type::storage_type> &&
                      !std::is_same_v<typename field_type::storage_type, typename field_type::value_type>) {
            if (opt_.field_comment.show_storage_type_when_different) {
                begin_or_separate_comment(started);
                write<text_color::comment>("storage=");
                write<text_color::comment>(contract::adapters::debug::type_name<typename field_type::storage_type>());
            }
        }

        if (opt_.field_comment.show_accessor_kind) {
            if constexpr (field_type::is_property_field) {
                begin_or_separate_comment(started);
                write<text_color::comment_provenance>("property");
            }

            if constexpr (field_type::template has_custom_get<const object_type&>) {
                begin_or_separate_comment(started);
                write<text_color::comment_provenance>("custom_get");
            }

            if constexpr (field_type::template has_custom_set<object_type&, value_type&&>) {
                begin_or_separate_comment(started);
                write<text_color::comment_provenance>("custom_set");
            }
        }

        if constexpr (field_type::is_base_import) {
            if (opt_.field_comment.show_base_offset) {
                begin_or_separate_comment(started);
                write<text_color::comment_provenance>(contract::adapters::debug::type_name<typename field_type::owner_type>());
                write<text_color::comment>("+");
                write<text_color::comment>(field_type::base_offset);
            }
        }

        if constexpr (codec_detail::has_write_comment_v<console::codec<value_type>, writer<Output>, Value>) {
            if (!redacted) {
                codec_detail::write_comment_if_present<console::codec<value_type>>(*this, value);
            }
        }

        if (opt_.field_comment.show_attributes) {
            std::apply(
                [this, &started](const auto&... entries) {
                    (([&] {
                        if (entries.source.empty()) {
                            return;
                        }

                        begin_or_separate_comment(started);
                        write<text_color::comment_attribute>(entries.source);
                    }()), ...);
                },
                descriptor.attributes.entries);
        }
    }

    template<class Sequence>
    void write_sequence(const Sequence& value) {
        const std::size_t size = std::size(value);
        const std::size_t shown = size < opt_.max_items ? size : opt_.max_items;

        std::size_t index = 0;
        for (const auto& item : value) {
            if (index == shown) {
                break;
            }

            write_indexed_item(index, item);

            ++index;
        }

        if (shown != size) {
            indent();
            write<text_color::comment>("- ... # ");
            write<text_color::comment>("truncated");
            write<text_color::comment>(", +");
            write<text_color::comment>(size - shown);
            write<text_color::comment>(" items");
            newline();
        }
    }

    void write_index_comment(std::size_t index, std::string_view type_name = {}) {
        if (opt_.output_mode != options::mode::schema || !opt_.show_indexes) {
            return;
        }

        record_comment_width();
        write_comment_padding();
        write<text_color::comment>(" # [");
        write<text_color::comment>(std::to_string(index));
        write<text_color::comment>("]");

        if (!type_name.empty()) {
            write<text_color::comment>(" ");
            write<text_color::comment>(type_name);
        }
    }

    void write_comment_padding() {
        if (!opt_.align_comments || opt_.output_mode != options::mode::schema) {
            return;
        }

        out_ << std::string(state_.next_comment_padding(), ' ');
    }

    void begin_or_separate_comment(bool& started) {
        if (started) {
            write<text_color::comment>(", ");
            return;
        }

        record_comment_width();
        write_comment_padding();
        write<text_color::comment>(" # ");
        started = true;
    }

public:
    void indent() {
        out_ << std::string(depth_ * opt_.indent, ' ');
    }

    template<class T>
    void write_scalar(const T& value) {
        if constexpr (std::is_same_v<contract::adapters::base::clean_t<T>, bool>) {
            write<text_color::number>(value ? "true" : "false");
        } else {
            write<text_color::number>(value);
        }
    }

private:
    template<text_color Color, class T>
    void write(const T& value) {
        const auto code = opt_.color.palette.*detail::palette_for<Color>::member;
        if (is_color_enabled(code)) {
            out_ << code;
            if constexpr (std::is_convertible_v<T, std::string_view>) {
                out_ << std::string_view(value);
            } else {
                out_ << value;
            }
            out_ << opt_.color.palette.reset;
        } else {
            if constexpr (std::is_convertible_v<T, std::string_view>) {
                out_ << std::string_view(value);
            } else {
                out_ << value;
            }
        }
    }

    void write_string(std::string_view value) {
        const auto truncated = contract::adapters::debug::truncate_string(value, opt_.max_string_length);
        write<text_color::string_literal>(contract::adapters::base::quoted_string(truncated.value));
        if (truncated.truncated()) {
            write<text_color::comment>(" # ");
            write<text_color::comment>("truncated");
            write<text_color::comment>(", +");
            write<text_color::comment>(std::to_string(truncated.omitted));
            write<text_color::comment>(" bytes");
        }
    }

public:
    void write_count_comment(std::string_view label, std::size_t count, bool truncated = false) {
        if (opt_.show_container_size) {
            write<text_color::comment>(", ");
            write<text_color::comment>(label);
            write<text_color::comment>("=");
            write<text_color::comment>(std::to_string(count));
            if (truncated) {
                write<text_color::comment>(", ");
                write<text_color::comment>("truncated");
            }
        }
    }

    std::size_t max_string_length() const {
        return opt_.max_string_length;
    }

    std::size_t max_byte_preview_length() const {
        return opt_.max_byte_preview_length;
    }

    template<std::size_t N>
    void write_bitset(const std::bitset<N>& value) {
        const auto bits = value.to_string();
        if (bits.size() <= opt_.max_string_length) {
            write<text_color::string_literal>(contract::adapters::base::quoted_string(bits));
            return;
        }

        std::string preview;
        if (opt_.max_string_length > 3) {
            preview = bits.substr(0, opt_.max_string_length - 3) + "...";
        } else {
            preview = bits.substr(0, opt_.max_string_length);
        }

        write<text_color::string_literal>(contract::adapters::base::quoted_string(preview));
    }

    template<class Byte>
    void write_bytes_preview(const Byte* data, std::size_t size) {
        const std::size_t trimmed = contract::adapters::base::trim_trailing_zeros(data, size);
        if (contract::adapters::debug::is_printable_ascii(data, trimmed)) {
            const std::string_view text{reinterpret_cast<const char*>(data), trimmed};
            const auto truncated = contract::adapters::debug::truncate_string(text, opt_.max_string_length);
            write<text_color::string_literal>(contract::adapters::base::quoted_string(truncated.value));
            return;
        }

        const auto preview = contract::adapters::debug::format_bytes_preview_hex(data, size, opt_.max_byte_preview_length);
        write<text_color::string_literal>(contract::adapters::base::quoted_string(preview));
    }

    // Mirrors write_bytes_preview's text-vs-hex decision so callers can report
    // one accurate "truncated" flag in their own comment, instead of guessing
    // from max_byte_preview_length alone (which does not apply to the text path).
    template<class Byte>
    bool bytes_preview_truncated(const Byte* data, std::size_t size) const {
        const std::size_t trimmed = contract::adapters::base::trim_trailing_zeros(data, size);
        if (contract::adapters::debug::is_printable_ascii(data, trimmed)) {
            return trimmed > opt_.max_string_length;
        }
        return size > opt_.max_byte_preview_length;
    }

    template<class T>
    void write_indexed_item(std::size_t index, const T& item, std::string_view type_name = {}) {
        indent();
        out_ << '-';

        using item_type = contract::adapters::base::clean_t<T>;
        if constexpr (console::codec<item_type>::block) {
            write_index_comment(index, type_name);
            newline();
            scoped_depth guard(*this);
            write_value(item);
        } else {
            out_ << ' ';
            write_value(item);
            write_index_comment(index, type_name);
            newline();
        }
    }

    template<class T>
    void write_named_value(std::string_view name, const T& value) {
        indent();
        write<text_color::field_name>(name);
        out_ << ':';

        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (console::codec<value_type>::block) {
            newline();
            scoped_depth guard(*this);
            write_value(value);
        } else {
            out_ << ' ';
            write_value(value);
            newline();
        }
    }

    template<class Key, class Value>
    void write_map_item(std::size_t index, const Key& key, const Value& value) {
        indent();
        write_value(key);
        out_ << ':';

        using value_type = contract::adapters::base::clean_t<Value>;
        if constexpr (console::codec<value_type>::block) {
            write_index_comment(index);
            newline();
            scoped_depth guard(*this);
            write_value(value);
        } else {
            out_ << ' ';
            write_value(value);
            write_index_comment(index);
            newline();
        }
    }

    template<class Key, class Value>
    void write_map_entry(std::size_t index, const Key& key, const Value& value) {
        using key_type = contract::adapters::base::clean_t<Key>;
        using value_type = contract::adapters::base::clean_t<Value>;

        if constexpr (contract::adapters::base::has_contract_definition<key_type> &&
                      contract::adapters::base::has_contract_definition<value_type>) {
            indent();
            out_ << "- ";
            write<text_color::type_name>(contract::adapters::debug::type_name<key_type>());
            out_ << ':';
            write_index_comment(index, "key");
            newline();
            scoped_depth guard(*this);
            write_contract_object_fields(key);

            indent();
            write<text_color::type_name>(contract::adapters::debug::type_name<value_type>());
            out_ << ':';
            write_index_comment(index, "value");
            newline();
            write_contract_object_fields(value);
        } else {
            indent();
            out_ << '-';
            write_index_comment(index);
            newline();
            scoped_depth guard(*this);
            write_named_value("key", key);
            write_named_value("value", value);
        }
    }

    void newline() {
        out_ << '\n';
    }

private:
    bool is_color_enabled(std::string_view code) const {
        return opt_.color.enabled && !code.empty();
    }

    static constexpr bool is_measurement_output() {
        using output_type = std::remove_reference_t<Output>;
        return std::is_same_v<output_type, detail::measurement_sink>;
    }

    bool needs_comment_layout() const {
        return opt_.align_comments &&
               opt_.output_mode == options::mode::schema &&
               !is_measurement_output();
    }

    template<class T>
    void prepare_comment_layout(const T& value) {
        options measurement_opt = opt_;
        measurement_opt.color.enabled = false;
        measurement_opt.align_comments = true;

        detail::measurement_sink sink;
        writer<detail::measurement_sink&> probe(sink, measurement_opt);
        probe.write_value(value);

        state_.comment_paddings = std::move(sink.comment_widths);

        const auto max_width = state_.comment_paddings.empty()
            ? std::size_t{0}
            : *std::max_element(state_.comment_paddings.begin(), state_.comment_paddings.end());

        for (auto& width : state_.comment_paddings) {
            width = max_width > width ? (max_width - width + 1) : 1;
        }

        state_.comment_cursor = 0;
    }

    void record_comment_width() {
        if constexpr (is_measurement_output()) {
            if (!opt_.align_comments || opt_.output_mode != options::mode::schema) {
                return;
            }

            out_.mark_comment();
        }
    }

    struct scoped_depth {
        explicit scoped_depth(writer& out)
            : out_(out) {
            ++out_.depth_;
        }

        ~scoped_depth() {
            --out_.depth_;
        }

    private:
        writer& out_;
    };

    output_type out_;
    options opt_;
    std::size_t depth_ = 0;
    detail::render_state state_{};
};

template<class T>
struct codec<T, std::enable_if_t<contract::adapters::base::has_contract_definition<T>, void>> {
    static constexpr bool block = true;

    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.write(value, detail::node_kind::contract_object);
    }

};

template<class T>
struct codec<T, std::enable_if_t<codec_detail::is_scalar_value_v<T>, void>> {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.write(value, detail::node_kind::scalar);
    }

};

template<>
struct codec<bool, void> {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer& out, bool value) {
        out.write(value ? "true" : "false", detail::node_kind::scalar);
    }

};

template<>
struct codec<std::string, void> {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer& out, const std::string& value) {
        out.write(value, detail::node_kind::string);
    }

};

template<>
struct codec<std::string_view, void> {
    static constexpr bool block = false;

    template<class Writer>
    static void write(Writer& out, std::string_view value) {
        out.write(value, detail::node_kind::string);
    }

};

template<class T>
std::string to_string(const T& value, options opt = {}) {
    std::ostringstream out;
    writer<std::ostringstream&>{out, opt} << value;
    return out.str();
}

} // namespace contract::adapters::console
