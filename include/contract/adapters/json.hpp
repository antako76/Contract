#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/adapters/debug/format.hpp>
#include <contract/contract.hpp>
#include <contract/security.hpp>

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace contract::adapters::json {

enum class security_mode {
    ignore,
    omit,
    redact,
};

struct options {
    security_mode no_log = security_mode::ignore;
    security_mode secret = security_mode::ignore;
    security_mode sensitive = security_mode::ignore;
};

// The second template parameter enables partial specializations for codec selection.
template<class T, class Enable = void>
struct codec;

namespace detail {

// Unlike contract::adapters::base::escape_string (a console-oriented, human
// readable convention that uses \xNN for control characters), JSON only
// recognizes \uXXXX - \xNN would emit invalid JSON. This escapes the same
// named characters but falls back to \u00NN for the rest.
inline std::string escape_json_string(std::string_view value) {
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
                out += "\\u00";
                out += hex[(ch >> 4) & 0x0f];
                out += hex[ch & 0x0f];
            } else {
                out.push_back(static_cast<char>(ch));
            }
        }
    }

    return out;
}

} // namespace detail

template<class Output = std::ostream&>
class writer {
public:
    explicit writer(Output out)
        : out_(out) {}

    explicit writer(Output out, options opt)
        : out_(out)
        , opt_(opt) {}

    template<class T>
    writer& operator<<(const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        codec<value_type>::write(*this, value);
        return *this;
    }

    template<class Field, class Object>
    void field(const Field& field, const Object& obj) {
        bool omit = false;
        bool redact = false;

        if constexpr (contract::has_attribute_v<Field, contract::security::no_log>) {
            if (opt_.no_log == security_mode::omit) {
                omit = true;
            } else if (opt_.no_log == security_mode::redact) {
                redact = true;
            }
        }

        if constexpr (contract::has_attribute_v<Field, contract::security::secret>) {
            if (opt_.secret == security_mode::omit) {
                omit = true;
            } else if (opt_.secret == security_mode::redact) {
                redact = true;
            }
        }

        if constexpr (contract::has_attribute_v<Field, contract::security::sensitive>) {
            if (opt_.sensitive == security_mode::omit) {
                omit = true;
            } else if (opt_.sensitive == security_mode::redact) {
                redact = true;
            }
        }

        if (omit) {
            return;
        }

        begin_member(field.name);

        if (redact) {
            write_string("<redacted>");
            return;
        }

        using value_type = contract::adapters::base::clean_t<decltype(field.get(obj))>;
        const auto& value = field.get(obj);
        using codec_type = codec<value_type>;

        if constexpr (base::has_field_write<codec_type, writer, Field, value_type>) {
            codec_type::write(*this, field, value);
        } else {
            codec_type::write(*this, value);
        }
    }

    template<class T>
    std::string to_string(const T& value) {
        std::ostringstream out;
        writer<std::ostringstream&> nested(out, opt_);
        nested << value;
        return out.str();
    }

    void begin_object() {
        out_ << '{';
        stack_.push_back({true});
    }

    void end_object() {
        out_ << '}';
        stack_.pop_back();
    }

    void begin_array() {
        out_ << '[';
        stack_.push_back({true});
    }

    void end_array() {
        out_ << ']';
        stack_.pop_back();
    }

    void begin_member(std::string_view name) {
        begin_value();
        write_string(name);
        out_ << ':';
    }

    void write_null() {
        out_ << "null";
    }

    void write_bool(bool value) {
        out_ << (value ? "true" : "false");
    }

    template<class T>
    void write_number(const T& value) {
        using value_type = contract::adapters::base::clean_t<T>;
        if constexpr (std::is_same_v<value_type, char> ||
                      std::is_same_v<value_type, signed char> ||
                      std::is_same_v<value_type, unsigned char>) {
            out_ << static_cast<int>(value);
        } else {
            out_ << value;
        }
    }

    void write_string(std::string_view value) {
        out_ << '"' << detail::escape_json_string(value) << '"';
    }

    template<class T>
    void write_optional(const T& value) {
        if (value.has_value()) {
            write_value(*value);
        } else {
            write_null();
        }
    }

    template<class T>
    void write_value(const T& value) {
        (*this) << value;
    }

    void begin_value() {
        if (stack_.empty()) {
            return;
        }

        if (!stack_.back().first) {
            out_ << ',';
        } else {
            stack_.back().first = false;
        }
    }

private:
    struct frame {
        bool first;
    };

    Output out_;
    options opt_{};
    std::vector<frame> stack_;
};

template<class T, class Enable>
struct codec {
    template<class Writer>
    static void write(Writer&, const T&) {
        static_assert(contract::adapters::base::always_false_v<T>,
            "json::codec<T> is not defined for this contract value type");
    }
};

template<class T>
struct codec<T, std::enable_if_t<contract::adapters::base::has_contract_definition<T>, void>> {
    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.begin_object();
        contract::visit(value, out);
        out.end_object();
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, std::nullptr_t>, void>> {
    template<class Writer>
    static void write(Writer& out, const T&) {
        out.write_null();
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, bool>, void>> {
    template<class Writer>
    static void write(Writer& out, bool value) {
        out.write_bool(value);
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_integral_v<contract::adapters::base::clean_t<T>> &&
                                 !std::is_same_v<contract::adapters::base::clean_t<T>, bool> &&
                                 !std::is_same_v<contract::adapters::base::clean_t<T>, char> &&
                                 !std::is_same_v<contract::adapters::base::clean_t<T>, signed char> &&
                                 !std::is_same_v<contract::adapters::base::clean_t<T>, unsigned char>, void>> {
    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.write_number(value);
    }
};

// `char` is treated as text (the ubiquitous C/C++ string element type);
// `signed char`/`unsigned char` are treated as numbers, matching the
// convention that explicitly choosing a signedness signals "this is a byte
// value", not text (e.g. OpenSSL-style unsigned char* buffers, hashes).
// This is a type-based decision, not a content scan: json is primarily used
// for the hot-path logger, so no per-value inspection is done here.
template<class T>
struct codec<T, std::enable_if_t<std::is_same_v<contract::adapters::base::clean_t<T>, char>, void>> {
    template<class Writer>
    static void write(Writer& out, char value) {
        out.write_string(std::string_view{&value, 1});
    }
};

template<class T>
struct codec<T, std::enable_if_t<
    std::is_same_v<contract::adapters::base::clean_t<T>, signed char> ||
    std::is_same_v<contract::adapters::base::clean_t<T>, unsigned char>, void>> {
    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.write_number(value);
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_floating_point_v<contract::adapters::base::clean_t<T>>, void>> {
    template<class Writer>
    static void write(Writer& out, const T& value) {
        out.write_number(value);
    }
};

template<class T>
struct codec<T, std::enable_if_t<std::is_enum_v<contract::adapters::base::clean_t<T>>, void>> {
    template<class Writer>
    static void write(Writer& out, const T& value) {
        using underlying_type = std::underlying_type_t<contract::adapters::base::clean_t<T>>;
        out.write_number(static_cast<underlying_type>(value));
    }
};

template<>
struct codec<std::string, void> {
    template<class Writer>
    static void write(Writer& out, const std::string& value) {
        out.write_string(value);
    }
};

template<>
struct codec<std::string_view, void> {
    template<class Writer>
    static void write(Writer& out, std::string_view value) {
        out.write_string(value);
    }
};

template<>
struct codec<const char*, void> {
    template<class Writer>
    static void write(Writer& out, const char* value) {
        if (value == nullptr) {
            out.write_null();
        } else {
            out.write_string(value);
        }
    }
};

template<class T>
std::string to_string(const T& value, options opt = {}) {
    std::ostringstream out;
    writer<std::ostringstream&> json(out, opt);
    json << value;
    return out.str();
}

} // namespace contract::adapters::json
