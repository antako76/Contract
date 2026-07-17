#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/adapters/base/format.hpp>
#include <contract/contract.hpp>
#include <contract/detail/error.hpp>
#include <contract/io/byte_window.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <exception>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace contract::adapters::yaml {

struct options {
    bool fail_on_unknown_keys = true;
    bool fail_on_duplicate_keys = true;
};

template<class T, class Enable = void>
struct codec;

namespace detail {

template<class T>
struct is_optional : std::false_type {};

template<class T>
struct is_optional<std::optional<T>> : std::true_type {};

template<class T>
inline constexpr bool is_optional_v = is_optional<contract::adapters::base::clean_t<T>>::value;

template<class T>
struct is_vector : std::false_type {};

template<class T, class Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template<class T>
inline constexpr bool is_vector_v = is_vector<contract::adapters::base::clean_t<T>>::value;

template<class T>
struct is_array : std::false_type {};

template<class T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};

template<class T>
inline constexpr bool is_array_v = is_array<contract::adapters::base::clean_t<T>>::value;

template<class T, class = void>
struct tuple_size : std::false_type {};

template<class T>
struct tuple_size<T, std::void_t<typename std::tuple_size<contract::adapters::base::clean_t<T>>::type>>
    : std::true_type {};

template<class T>
inline constexpr bool is_tuple_like_v = tuple_size<T>::value && !is_array_v<T>;

template<class T>
inline constexpr bool is_contract_v = contract::adapters::base::has_contract_definition<contract::adapters::base::clean_t<T>>;

template<class T>
inline constexpr bool is_block_value_v =
    is_contract_v<T> ||
    is_vector_v<T> ||
    is_array_v<T> ||
    is_tuple_like_v<T>;

inline bool is_space(char ch) {
    return ch == ' ' || ch == '\t';
}

inline std::string_view trim_left(std::string_view value) {
    while (!value.empty() && is_space(value.front())) {
        value.remove_prefix(1);
    }
    return value;
}

inline std::string_view trim_right(std::string_view value) {
    while (!value.empty() && is_space(value.back())) {
        value.remove_suffix(1);
    }
    return value;
}

inline std::string_view trim(std::string_view value) {
    return trim_right(trim_left(value));
}

inline std::string_view strip_trailing_comment(std::string_view value) {
    bool in_single = false;
    bool in_double = false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch == '\'' && !in_double) {
            in_single = !in_single;
        } else if (ch == '"' && !in_single) {
            in_double = !in_double;
        } else if (ch == '#' && !in_single && !in_double) {
            if (i == 0 || is_space(value[i - 1])) {
                return trim_right(value.substr(0, i));
            }
        }
    }
    return value;
}

inline std::string_view normalize_scalar_token(std::string_view value) {
    return trim(strip_trailing_comment(value));
}

inline bool is_quoted(std::string_view value) {
    return value.size() >= 2 &&
           ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\''));
}

inline bool unescape_double_quoted(std::string_view value, std::string& result) {
    result.clear();
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch != '\\') {
            result.push_back(ch);
            continue;
        }

        if (i + 1 >= value.size()) {
            return false;
        }

        const char escaped = value[++i];
        switch (escaped) {
        case '\\': result.push_back('\\'); break;
        case '"': result.push_back('"'); break;
        case 'n': result.push_back('\n'); break;
        case 'r': result.push_back('\r'); break;
        case 't': result.push_back('\t'); break;
        default:
            return false;
        }
    }

    return true;
}

inline bool parse_string(std::string_view token, std::string& value) {
    if (token.empty()) {
        value.clear();
        return true;
    }
    if (!is_quoted(token)) {
        value = std::string(token);
        return true;
    }
    if (token.front() == '"') {
        return unescape_double_quoted(token.substr(1, token.size() - 2), value);
    }
    value = std::string(token.substr(1, token.size() - 2));
    return true;
}

inline bool unquote_string_view(std::string_view token, std::string_view& value) {
    if (!is_quoted(token)) {
        value = token;
        return true;
    }

    const std::string_view body = token.substr(1, token.size() - 2);
    if (token.front() == '"') {
        if (body.find('\\') != std::string_view::npos) {
            return false;
        }
        value = body;
        return true;
    }

    if (body.find('\'') != std::string_view::npos) {
        return false;
    }
    value = body;
    return true;
}

template<class T>
inline bool parse_integral(T& value, std::string_view token) {
    T parsed{};
    const auto* begin = token.data();
    const auto* end = token.data() + token.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }
    value = parsed;
    return true;
}

template<class T>
inline bool parse_floating_point(T& value, std::string_view token) {
    std::string text(token);
    char* last = nullptr;
    const auto parsed = std::strtod(text.c_str(), &last);
    if (last == nullptr || last != text.c_str() + text.size()) {
        return false;
    }
    value = static_cast<T>(parsed);
    return true;
}

} // namespace detail

using parse_status = contract::adapters::base::status;

enum class parse_error_code {
    unknown,
    expected_scalar_value,
    unexpected_indentation,
    expected_colon,
    missing_current_value,
    expected_nested_mapping,
    missing_nested_mapping,
    expected_nested_sequence,
    missing_nested_sequence,
    unexpected_nested_block,
    duplicate_key,
    unknown_key,
    missing_required_key,
    tabs_not_supported,
    internal_scanner_state_mismatch,
    invalid_string_scalar,
    expected_boolean_scalar,
    expected_null_scalar,
    expected_integral_scalar,
    expected_floating_point_scalar,
    expected_enum_scalar,
    too_many_sequence_items,
    sequence_length_mismatch,
    sequence_index_out_of_range,
    unexpected_trailing_content,
};

enum class parse_stage {
    none,
    root,
    scalar,
    mapping,
    sequence,
    optional,
    scanner,
    field,
};

inline std::string_view to_string(parse_error_code value) noexcept {
    switch (value) {
    case parse_error_code::unknown:
        return "unknown";
    case parse_error_code::expected_scalar_value:
        return "expected scalar value";
    case parse_error_code::unexpected_indentation:
        return "unexpected indentation";
    case parse_error_code::expected_colon:
        return "expected ':' in mapping entry";
    case parse_error_code::missing_current_value:
        return "missing current value";
    case parse_error_code::expected_nested_mapping:
        return "expected nested mapping for contract object";
    case parse_error_code::missing_nested_mapping:
        return "missing nested mapping";
    case parse_error_code::expected_nested_sequence:
        return "expected nested sequence";
    case parse_error_code::missing_nested_sequence:
        return "missing nested sequence";
    case parse_error_code::unexpected_nested_block:
        return "unexpected nested block for optional scalar";
    case parse_error_code::duplicate_key:
        return "duplicate key";
    case parse_error_code::unknown_key:
        return "unknown key";
    case parse_error_code::missing_required_key:
        return "missing required key";
    case parse_error_code::tabs_not_supported:
        return "tabs are not supported";
    case parse_error_code::internal_scanner_state_mismatch:
        return "internal scanner state mismatch";
    case parse_error_code::invalid_string_scalar:
        return "invalid string scalar";
    case parse_error_code::expected_boolean_scalar:
        return "expected boolean scalar";
    case parse_error_code::expected_null_scalar:
        return "expected null scalar";
    case parse_error_code::expected_integral_scalar:
        return "expected integral scalar";
    case parse_error_code::expected_floating_point_scalar:
        return "expected floating-point scalar";
    case parse_error_code::expected_enum_scalar:
        return "expected enum scalar";
    case parse_error_code::too_many_sequence_items:
        return "too many sequence items";
    case parse_error_code::sequence_length_mismatch:
        return "sequence length mismatch";
    case parse_error_code::sequence_index_out_of_range:
        return "sequence index out of range";
    case parse_error_code::unexpected_trailing_content:
        return "unexpected trailing content";
    }
    return "unknown";
}

inline std::string_view to_string(parse_stage value) noexcept {
    switch (value) {
    case parse_stage::none:
        return "parse";
    case parse_stage::root:
        return "root";
    case parse_stage::scalar:
        return "scalar";
    case parse_stage::mapping:
        return "mapping";
    case parse_stage::sequence:
        return "sequence";
    case parse_stage::optional:
        return "optional";
    case parse_stage::scanner:
        return "scanner";
    case parse_stage::field:
        return "field";
    }
    return "parse";
}

struct parse_error
    : std::exception
    , contract::detail::adapter_error_base<parse_error, parse_error_code, parse_stage, parse_status> {
public:
    using contract::detail::adapter_error_base<parse_error, parse_error_code, parse_stage, parse_status>::adapter_error_base;

    explicit parse_error(
        std::size_t line = 0,
        std::string_view snippet = {},
        std::source_location location = std::source_location::current())
        : contract::detail::adapter_error_base<parse_error, parse_error_code, parse_stage, parse_status>(location) {
        if (line != 0) {
            line_ = line;
        }
        if (!snippet.empty()) {
            snippet_ = std::string(snippet);
        }
    }

    parse_error& line(std::size_t value) noexcept {
        if (!line_) {
            line_ = value;
        }
        return *this;
    }

    std::string message() const {
        return base_message("yaml reader", "reading", parse_stage::none);
    }

    const char* what() const noexcept override {
        cache_ = message();
        return cache_.c_str();
    }

private:
    friend class contract::detail::adapter_error_base<parse_error, parse_error_code, parse_stage, parse_status>;

    void append_adapter_details(std::string& message) const {
        if (line_) {
            message += " at line ";
            message += std::to_string(*line_);
        }

        if (!snippet_.empty()) {
            constexpr std::size_t max_len = 48;
            std::string escaped = contract::adapters::base::escape_string(
                snippet_.substr(0, std::min<std::size_t>(snippet_.size(), max_len)));
            if (snippet_.size() > max_len) {
                escaped += "...";
            }

            message += " near \"";
            message += escaped;
            message += "\"";
        }
    }

    mutable std::string cache_{};
    std::optional<std::size_t> line_{};
    std::string snippet_{};
};

template<class Input = contract::io::window_input>
class reader {
public:
    static_assert(contract::io::has_window_input<Input>,
        "yaml reader requires a window_input-like backend");

    enum class optional_state {
        empty_or_null,
        nested_block,
        value,
    };

    explicit reader(Input input, options opt = {})
        : scanner_(*this, std::move(input))
        , opt_(opt) {}

    template<class T>
    reader& operator>>(T& value) {
        if (read(value) == parse_status::error) {
            throw *error_;
        }
        return *this;
    }

    template<class T>
    parse_status read(T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        error_.reset();

        if (codec<value_type>::read(*this, value) == parse_status::error) {
            return parse_status::error;
        }

        const auto line = scanner_.peek_line();
        if (error_) {
            return parse_status::error;
        }
        if (line) {
            return error()
                .code(parse_error_code::unexpected_trailing_content)
                .stage(parse_stage::root);
        }

        return parse_status::ok;
    }

    const std::optional<parse_error>& error() const noexcept {
        return error_;
    }

    parse_error& error(const parse_error& child) noexcept {
        return error().transfer_from(child);
    }

    parse_error& error(std::source_location location = std::source_location::current()) {
        if (!error_) {
            const auto location_info = current_error_location();
            error_.emplace(location_info.line, location_info.snippet, location);
        }
        return *error_;
    }

    parse_status read_scalar_token(std::string_view& token) {
        if (current_value_.active) {
            token = current_value_.inline_value;
            return parse_status::ok;
        }

        auto line_opt = scanner_.peek_line();
        if (error_) {
            return parse_status::error;
        }
        if (!line_opt) {
            return error()
                .code(parse_error_code::expected_scalar_value)
                .stage(parse_stage::root);
        }

        const auto line = *line_opt;
        if (line.indent != 0) {
            return error()
                .code(parse_error_code::unexpected_indentation)
                .stage(parse_stage::root);
        }
        if (!line.text.empty() && line.text.front() == '-') {
            return error()
                .code(parse_error_code::expected_scalar_value)
                .stage(parse_stage::root);
        }
        if (!line.text.empty() && !detail::is_quoted(line.text) && line.text.find(':') != std::string_view::npos) {
            return error()
                .code(parse_error_code::expected_scalar_value)
                .stage(parse_stage::root);
        }

        if (scanner_.consume_line() == parse_status::error) {
            return parse_status::error;
        }
        token = line.text;
        return parse_status::ok;
    }

    parse_status peek_optional_state(optional_state& state) {
        if (current_value_.active) {
            const auto value = current_value_.inline_value;
            if (value == "null" || value == "~") {
                state = optional_state::empty_or_null;
                return parse_status::ok;
            }
            if (value.empty()) {
                state = has_current_child_block()
                    ? optional_state::nested_block
                    : optional_state::empty_or_null;
                return error_ ? parse_status::error : parse_status::ok;
            }
            state = optional_state::value;
            return parse_status::ok;
        }

        auto line_opt = scanner_.peek_line();
        if (error_) {
            return parse_status::error;
        }
        if (!line_opt) {
            state = optional_state::empty_or_null;
            return parse_status::ok;
        }

        const auto line = *line_opt;
        if (line.indent == 0 && (line.text == "null" || line.text == "~")) {
            if (scanner_.consume_line() == parse_status::error) {
                return parse_status::error;
            }
            state = optional_state::empty_or_null;
            return parse_status::ok;
        }

        state = optional_state::value;
        return parse_status::ok;
    }

    template<class T>
    parse_status read_current_value(T& value) {
        if (!current_value_.active) {
            return error()
                .code(parse_error_code::missing_current_value)
                .stage(parse_stage::field);
        }

        using value_type = contract::adapters::base::clean_t<T>;
        value_scope scope{*this};
        return codec<value_type>::read(*this, value);
    }

    parse_status begin_mapping(std::size_t& indent) {
        return begin_block(parse_error_code::expected_nested_mapping, parse_error_code::missing_nested_mapping, parse_stage::mapping, indent);
    }

    parse_status begin_sequence(parse_error_code expected_inline_error, parse_error_code missing_error, std::size_t& indent) {
        return begin_block(expected_inline_error, missing_error, parse_stage::sequence, indent);
    }

    parse_status read_mapping_entry(std::size_t indent, std::string_view& key, bool& present) {
        present = false;

        auto line_opt = scanner_.peek_line();
        if (error_) {
            return parse_status::error;
        }
        if (!line_opt) {
            return parse_status::ok;
        }

        const auto line = *line_opt;
        if (line.indent < indent) {
            return parse_status::ok;
        }
        if (line.indent > indent) {
            return error()
                .code(parse_error_code::unexpected_indentation)
                .stage(parse_stage::mapping);
        }
        if (line.text.empty() || line.text.front() == '-') {
            return parse_status::ok;
        }

        const auto colon = line.text.find(':');
        if (colon == std::string_view::npos) {
            return error()
                .code(parse_error_code::expected_colon)
                .stage(parse_stage::mapping);
        }

        if (scanner_.consume_line() == parse_status::error) {
            return parse_status::error;
        }
        set_current_value(indent, detail::trim(line.text.substr(colon + 1)), line.line_no);
        key = detail::trim(line.text.substr(0, colon));
        present = true;
        return parse_status::ok;
    }

    parse_status read_sequence_item(std::size_t indent, bool& present) {
        present = false;

        auto line_opt = scanner_.peek_line();
        if (error_) {
            return parse_status::error;
        }
        if (!line_opt) {
            return parse_status::ok;
        }

        const auto line = *line_opt;
        if (line.indent < indent) {
            return parse_status::ok;
        }
        if (line.indent > indent) {
            return error()
                .code(parse_error_code::unexpected_indentation)
                .stage(parse_stage::sequence);
        }
        if (line.text.empty() || line.text.front() != '-') {
            return parse_status::ok;
        }

        if (scanner_.consume_line() == parse_status::error) {
            return parse_status::error;
        }
        set_current_value(indent, detail::trim(line.text.substr(1)), line.line_no);
        present = true;
        return parse_status::ok;
    }

private:
    struct line_info {
        std::size_t line_no;
        std::size_t indent;
        std::string_view text;
    };

    struct error_location {
        std::size_t line = 0;
        std::string_view snippet{};
    };

    error_location current_error_location() const {
        if (current_value_.active) {
            return {current_value_.line_no, current_value_.inline_value};
        }
        if (auto line = scanner_.peek_line()) {
            return {line->line_no, line->text};
        }
        if (auto line = scanner_.last_line()) {
            return {line->line_no, line->text};
        }
        return {scanner_.current_line_no(), {}};
    }

    parse_status begin_block(
        parse_error_code expected_inline_error,
        parse_error_code missing_error,
        parse_stage stage,
        std::size_t& indent)
    {
        if (!current_value_empty()) {
            return error()
                .code(expected_inline_error)
                .stage(stage);
        }
        if (current_value_.active && !has_current_child_block()) {
            if (error_) {
                return parse_status::error;
            }
            return error()
                .code(missing_error)
                .stage(stage);
        }
        indent = current_child_indent();
        return parse_status::ok;
    }

    template<class Source>
    class scanner {
    public:
        explicit scanner(reader& owner, Source source)
            : owner_(&owner)
            , source_(std::move(source)) {}

        std::optional<line_info> peek_line() const {
            if (!cached_) {
                cached_line_ = read_next_content_line();
                cached_ = true;
            }
            return cached_line_;
        }

        parse_status consume_line() {
            auto line = peek_line();
            if (!line) {
                return owner_->error(std::source_location::current())
                    .code(parse_error_code::internal_scanner_state_mismatch)
                    .stage(parse_stage::scanner);
            }
            last_line_ = cached_line_;
            cached_line_.reset();
            cached_ = false;
            return parse_status::ok;
        }

        std::size_t current_line_no() const {
            const auto line = peek_line();
            if (line) {
                return line->line_no;
            }
            if (last_line_) {
                return last_line_->line_no;
            }
            return next_line_no_ == 1 ? 1 : next_line_no_ - 1;
        }

        std::optional<line_info> last_line() const {
            return last_line_;
        }

    private:
        bool read_physical_line(std::string& line, std::size_t& line_no) const {
            line.clear();
            line_no = next_line_no_;

            bool has_data = false;
            while (true) {
                const auto window = source_.peek(4096);
                if (window.empty()) {
                    if (!has_data && line.empty()) {
                        return false;
                    }
                    ++next_line_no_;
                    return true;
                }

                has_data = true;

                std::size_t end = 0;
                while (end < window.size()) {
                    const char ch = std::to_integer<char>(window[end]);
                    if (ch == '\n' || ch == '\r') {
                        append_window(line, window.subspan(0, end));
                        source_.consume(end + 1);
                        if (ch == '\r') {
                            const auto next = source_.peek(1);
                            if (!next.empty() && std::to_integer<char>(next.front()) == '\n') {
                                source_.consume(1);
                            }
                        }
                        ++next_line_no_;
                        return true;
                    }
                    ++end;
                }

                append_window(line, window);
                source_.consume(window.size());
            }
        }

        std::optional<line_info> read_next_content_line() const {
            std::string line;
            std::size_t line_no = 0;

            while (read_physical_line(line, line_no)) {
                std::size_t indent = 0;
                while (indent < line.size() && line[indent] == ' ') {
                    ++indent;
                }
                if (indent < line.size() && line[indent] == '\t') {
                    owner_->error(std::source_location::current())
                        .code(parse_error_code::tabs_not_supported)
                        .stage(parse_stage::scanner)
                        .line(line_no);
                    return std::nullopt;
                }

                const std::string_view raw{line.data() + indent, line.size() - indent};
                const auto normalized = detail::normalize_scalar_token(raw);
                if (normalized.empty()) {
                    continue;
                }

                retained_lines_.emplace_back(normalized);
                return line_info{line_no, indent, retained_lines_.back()};
            }

            return std::nullopt;
        }

        static void append_window(std::string& line, std::span<const std::byte> window) {
            line.reserve(line.size() + window.size());
            for (std::byte byte : window) {
                line.push_back(std::to_integer<char>(byte));
            }
        }

        reader* owner_;
        mutable Source source_;
        mutable std::size_t next_line_no_ = 1;
        mutable bool cached_ = false;
        mutable std::optional<line_info> cached_line_{};
        mutable std::optional<line_info> last_line_{};
        // Normalized lines back std::string_view values returned by codecs.
        mutable std::deque<std::string> retained_lines_;
    };

    struct value_state {
        bool active = false;
        std::size_t parent_indent = 0;
        std::size_t line_no = 1;
        std::string_view inline_value{};
    };

    class value_scope {
    public:
        explicit value_scope(reader& owner)
            : owner_(owner)
            , previous_(owner.current_value_) {}

        ~value_scope() {
            owner_.current_value_ = previous_;
        }

    private:
        reader& owner_;
        value_state previous_;
    };

    void set_current_value(std::size_t parent_indent, std::string_view inline_value, std::size_t line_no) {
        current_value_ = value_state{true, parent_indent, line_no, inline_value};
    }

    std::size_t current_child_indent() const {
        return current_value_.active ? current_value_.parent_indent + 2 : 0;
    }

    bool has_current_child_block() const {
        auto next = scanner_.peek_line();
        return next && next->indent >= current_child_indent();
    }

    bool current_value_empty() const {
        return !current_value_.active || current_value_.inline_value.empty();
    }

    mutable scanner<Input> scanner_;
    std::optional<parse_error> error_{};
    options opt_{};
    value_state current_value_{};

    template<class, class>
    friend struct codec;
};

template<class T, class Enable>
struct codec {
    template<class Reader>
    static parse_status read(Reader&, T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "yaml::codec<T> is not defined for this contract value type");
        return parse_status::error;
    }
};

template<class T>
struct codec<T, std::enable_if_t<contract::adapters::base::has_contract_definition<T>, void>> {
    template<class Reader>
    static parse_status read(Reader& in, T& value) {
        std::size_t indent = 0;
        if (in.begin_mapping(indent) == parse_status::error) {
            return parse_status::error;
        }
        return read_body(in, value, indent);
    }

private:
    template<class Reader>
    static parse_status read_body(Reader& in, T& value, std::size_t indent) {
        using object_type = contract::adapters::base::clean_t<T>;
        constexpr std::size_t field_count = contract::field_count<object_type>();
        std::array<bool, field_count> seen{};

        while (true) {
            std::string_view entry;
            bool present = false;
            if (in.read_mapping_entry(indent, entry, present) == parse_status::error) {
                return parse_status::error;
            }
            if (!present) {
                break;
            }

            parse_status field_status = parse_status::ok;
            const bool matched = contract::dispatch_field_by_name<object_type>(
                entry,
                [&in, &value, &seen, &field_status](const auto& field, std::size_t index) {
                    field_status = read_matched_field(in, value, field, seen, index);
                });

            if (!matched) {
                if (in.opt_.fail_on_unknown_keys) {
                    return in.error()
                        .code(parse_error_code::unknown_key)
                        .stage(parse_stage::field);
                }
            } else if (field_status == parse_status::error) {
                return field_status;
            }
        }

        auto fields = contract::flattened_fields_of<object_type>();
        return check_missing_fields(in, value, fields, seen, std::make_index_sequence<field_count>{});
    }

    template<class Reader, class Object, class Tuple, std::size_t... I>
    static parse_status check_missing_fields(
        Reader& in,
        Object& obj,
        Tuple& fields,
        const std::array<bool, sizeof...(I)>& seen,
        std::index_sequence<I...>)
    {
        parse_status status = parse_status::ok;
        ((status == parse_status::ok
              ? status = check_missing_field<I>(in, obj, std::get<I>(fields), seen[I])
              : status), ...);
        return status;
    }

    template<class Reader, class Object, class Field, std::size_t N>
    static parse_status read_matched_field(
        Reader& in,
        Object& obj,
        const Field& field,
        std::array<bool, N>& seen,
        std::size_t index)
    {
        using field_type = contract::adapters::base::clean_t<Field>;
        using value_type = typename field_type::value_type;

        if (seen[index] && in.opt_.fail_on_duplicate_keys) {
            return in.error()
                .code(parse_error_code::duplicate_key)
                .stage(parse_stage::field);
        }
        seen[index] = true;

        if constexpr (field_type::template can_direct_ref<Object>) {
            auto& ref = field.ref(obj);
            const auto status = in.read_current_value(ref);
            if (status == parse_status::error) {
                return in.error()
                    .type_name(contract::type_name<Object>())
                    .field(field);
            }
            return status;
        } else {
            value_type tmp{};
            if (in.read_current_value(tmp) == parse_status::error) {
                return in.error()
                    .type_name(contract::type_name<Object>())
                    .field(field);
            }
            field.set(obj, std::move(tmp));
            return parse_status::ok;
        }
    }

    template<std::size_t I, class Reader, class Object, class Field>
    static parse_status check_missing_field(Reader& in, Object& obj, const Field& field, bool seen) {
        using field_type = contract::adapters::base::clean_t<Field>;
        using value_type = typename field_type::value_type;

        if (!seen) {
            if constexpr (detail::is_optional_v<value_type>) {
                if constexpr (field_type::template can_direct_ref<Object>) {
                    field.ref(obj) = value_type{};
                } else {
                    field.set(obj, value_type{});
                }
            } else {
                return in.error()
                    .code(parse_error_code::missing_required_key)
                    .stage(parse_stage::field);
            }
        }
        return parse_status::ok;
    }
};

template<class T>
struct codec<std::optional<T>, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::optional<T>& value) {
        using inner_type = T;
        using optional_state = typename Reader::optional_state;

        optional_state state{};
        if (in.peek_optional_state(state) == parse_status::error) {
            return parse_status::error;
        }
        if (state == optional_state::empty_or_null) {
            value.reset();
            return parse_status::ok;
        }

        if (state == optional_state::nested_block) {
            if constexpr (detail::is_block_value_v<inner_type>) {
                value.emplace();
                return codec<inner_type>::read(in, *value);
            } else {
                return in.error()
                    .code(parse_error_code::unexpected_nested_block)
                    .stage(parse_stage::optional);
            }
        }

        value.emplace();
        return codec<T>::read(in, *value);
    }
};

template<class T>
struct codec<std::vector<T>, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::vector<T>& value) {
        std::size_t indent = 0;
        if (in.begin_sequence(
                parse_error_code::expected_nested_sequence,
                parse_error_code::missing_nested_sequence,
                indent) == parse_status::error) {
            return parse_status::error;
        }
        value.clear();
        while (true) {
            bool present = false;
            if (in.read_sequence_item(indent, present) == parse_status::error) {
                return parse_status::error;
            }
            if (!present) {
                break;
            }
            T element{};
            if (in.read_current_value(element) == parse_status::error) {
                return parse_status::error;
            }
            value.push_back(std::move(element));
        }
        return parse_status::ok;
    }
};

template<class T, std::size_t N>
struct codec<std::array<T, N>, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::array<T, N>& value) {
        std::size_t indent = 0;
        if (in.begin_sequence(
                parse_error_code::expected_nested_sequence,
                parse_error_code::missing_nested_sequence,
                indent) == parse_status::error) {
            return parse_status::error;
        }
        constexpr std::size_t size = N;
        std::size_t index = 0;
        while (true) {
            bool present = false;
            if (in.read_sequence_item(indent, present) == parse_status::error) {
                return parse_status::error;
            }
            if (!present) {
                break;
            }
            if (index >= size) {
                return in.error()
                    .code(parse_error_code::too_many_sequence_items)
                    .stage(parse_stage::sequence);
            }
            if (in.read_current_value(value[index]) == parse_status::error) {
                return parse_status::error;
            }
            ++index;
        }
        if (index != size) {
            return in.error()
                .code(parse_error_code::sequence_length_mismatch)
                .stage(parse_stage::sequence);
        }
        return parse_status::ok;
    }
};

template<class... Ts>
struct codec<std::tuple<Ts...>, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::tuple<Ts...>& value) {
        std::size_t indent = 0;
        if (in.begin_sequence(
                parse_error_code::expected_nested_sequence,
                parse_error_code::missing_nested_sequence,
                indent) == parse_status::error) {
            return parse_status::error;
        }
        constexpr std::size_t size = sizeof...(Ts);
        std::size_t index = 0;
        while (true) {
            bool present = false;
            if (in.read_sequence_item(indent, present) == parse_status::error) {
                return parse_status::error;
            }
            if (!present) {
                break;
            }
            if (index >= size) {
                return in.error()
                    .code(parse_error_code::too_many_sequence_items)
                    .stage(parse_stage::sequence);
            }

            if (read_tuple_element<0>(in, value, index) == parse_status::error) {
                return parse_status::error;
            }
            ++index;
        }
        if (index != size) {
            return in.error()
                .code(parse_error_code::sequence_length_mismatch)
                .stage(parse_stage::sequence);
        }
        return parse_status::ok;
    }

    template<std::size_t I, class Reader>
    static parse_status read_tuple_element(Reader& in, std::tuple<Ts...>& value, std::size_t index) {
        if constexpr (I < sizeof...(Ts)) {
            if (index == I) {
                using element_type = std::tuple_element_t<I, std::tuple<Ts...>>;
                return in.read_current_value(std::get<I>(value));
            }
            return read_tuple_element<I + 1>(in, value, index);
        } else {
            return in.error()
                .code(parse_error_code::sequence_index_out_of_range)
                .stage(parse_stage::sequence);
        }
    }
};

template<>
struct codec<std::string, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::string& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (!detail::parse_string(token, value)) {
            return in.error()
                .code(parse_error_code::invalid_string_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<>
struct codec<std::string_view, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::string_view& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (!detail::unquote_string_view(token, value)) {
            return in.error()
                .code(parse_error_code::invalid_string_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<>
struct codec<bool, void> {
    template<class Reader>
    static parse_status read(Reader& in, bool& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (token == "true") {
            value = true;
        } else if (token == "false") {
            value = false;
        } else {
            return in.error()
                .code(parse_error_code::expected_boolean_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<>
struct codec<std::nullptr_t, void> {
    template<class Reader>
    static parse_status read(Reader& in, std::nullptr_t& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (token == "null" || token == "~") {
            value = nullptr;
        } else {
            return in.error()
                .code(parse_error_code::expected_null_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_integral_v<T> &&
                                 !std::is_same_v<T, bool> &&
                                 !std::is_same_v<T, char> &&
                                 !std::is_same_v<T, signed char> &&
                                 !std::is_same_v<T, unsigned char>, void>> {
    template<class Reader>
    static parse_status read(Reader& in, T& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (!detail::parse_integral(value, token)) {
            return in.error()
                .code(parse_error_code::expected_integral_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_floating_point_v<T>, void>> {
    template<class Reader>
    static parse_status read(Reader& in, T& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        if (!detail::parse_floating_point(value, token)) {
            return in.error()
                .code(parse_error_code::expected_floating_point_scalar)
                .stage(parse_stage::scalar);
        }
        return parse_status::ok;
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_enum_v<T>, void>> {
    template<class Reader>
    static parse_status read(Reader& in, T& value) {
        std::string_view token;
        if (in.read_scalar_token(token) == parse_status::error) {
            return parse_status::error;
        }
        using underlying_type = std::underlying_type_t<T>;
        underlying_type parsed{};
        if (!detail::parse_integral(parsed, token)) {
            return in.error()
                .code(parse_error_code::expected_enum_scalar)
                .stage(parse_stage::scalar);
        }
        value = static_cast<T>(parsed);
        return parse_status::ok;
    }
};

} // namespace contract::adapters::yaml
