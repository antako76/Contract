#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <source_location>

namespace contract::detail {

enum class field_kind {
    unknown,
    member,
    reference,
    property,
    offset,
};

struct error_field {
    field_kind kind = field_kind::unknown;
    std::string_view name{};
    std::uint32_t number = 0;
    std::optional<int> base_offset{};

    template<class Field>
    error_field& from(const Field& value) noexcept {
        if constexpr (requires { value.name; }) {
            if (name.empty()) {
                name = value.name;
            }
        }

        if constexpr (requires { value.id; }) {
            if (number == 0) {
                number = static_cast<std::uint32_t>(value.id);
            }
        }

        if constexpr (requires { Field::is_base_import; Field::is_member_field; Field::is_reference_field; Field::is_property_field; }) {
            if (kind == field_kind::unknown) {
                if constexpr (Field::is_base_import) {
                    kind = field_kind::offset;
                } else if constexpr (Field::is_member_field) {
                    kind = field_kind::member;
                } else if constexpr (Field::is_reference_field) {
                    kind = field_kind::reference;
                } else if constexpr (Field::is_property_field) {
                    kind = field_kind::property;
                } else {
                    kind = field_kind::unknown;
                }
            }
        }

        if constexpr (requires { Field::is_base_import; Field::base_offset; }) {
            if constexpr (Field::is_base_import) {
                if (!base_offset) {
                    base_offset = Field::base_offset;
                }
            }
        }
        return *this;
    }
};

static std::string_view field_kind_name(field_kind value) noexcept {
    switch (value) {
    case field_kind::unknown:
        return "field";
    case field_kind::member:
        return "member";
    case field_kind::reference:
        return "reference";
    case field_kind::property:
        return "property";
    case field_kind::offset:
        return "offset";
    }
    return "field";
}

template<class Derived, class Code, class Stage, class Status>
class adapter_error_base {
public:
    explicit adapter_error_base(std::source_location location = std::source_location::current()) noexcept
        : source_location_(location) {}

    operator Status() const noexcept {
        return Status::error;
    }

    Derived& type_name(std::string_view value) noexcept {
        if (!type_name_) {
            type_name_ = value;
        }
        return self();
    }

    Derived& code(Code value) noexcept {
        code_ = value;
        return self();
    }

    template<class Field>
    Derived& field(const Field& value) noexcept {
        if (!field_) {
            field_.emplace();
        }
        field_->from(value);
        return self();
    }

    Derived& stage(Stage value) noexcept {
        if (!stage_) {
            stage_ = value;
        }
        return self();
    }

    Derived& offset(std::size_t value) noexcept {
        if (!offset_) {
            offset_ = value;
        }
        return self();
    }

    Derived& position_correction(std::size_t base) noexcept {
        if (offset_) {
            *offset_ += base;
        } else {
            offset_ = base;
        }
        return self();
    }

    Derived& transfer_from(const Derived& value) noexcept {
        self() = value;
        return self();
    }

    Derived& element_index(std::size_t index) noexcept {
        if (!element_index_) {
            element_index_ = index;
        }
        return self();
    }

    Derived& element_index(std::size_t index, std::size_t total) noexcept {
        if (!element_index_) {
            element_index_ = index;
        }
        if (!element_total_) {
            element_total_ = total;
        }
        return self();
    }

    Derived& sizes(std::size_t expected, std::size_t actual) noexcept {
        if (!expected_size_) {
            expected_size_ = expected;
        }
        if (!actual_size_) {
            actual_size_ = actual;
        }
        return self();
    }

protected:
    std::string base_message(std::string_view owner, std::string_view action, Stage default_stage) const {
        std::string message;
        message += owner;
        message += ": ";
        message += to_string(code_);

        message += " while ";
        message += action;
        message += " ";
        message += to_string(stage_.value_or(default_stage));

        append_common_context(message);
        self().append_adapter_details(message);
        append_source_location(message);
        return message;
    }

private:
    Derived& self() noexcept {
        return static_cast<Derived&>(*this);
    }

    const Derived& self() const noexcept {
        return static_cast<const Derived&>(*this);
    }

    void append_common_context(std::string& message) const {
        if (type_name_) {
            message += " in ";
            message += *type_name_;
        }

        if (field_) {
            if (!field_->name.empty()) {
                message += " field ";
                message += field_->name;
                message += " (#";
                message += std::to_string(field_->number);
                message += ")";
            } else if (field_->number != 0) {
                message += " field #";
                message += std::to_string(field_->number);
            }
        }

        if (field_) {
            if (field_->kind != field_kind::unknown) {
                message += " [";
                message += field_kind_name(field_->kind);
                message += "]";
            }
        }

        if (field_ && field_->base_offset) {
            message += " base_offset ";
            message += std::to_string(*field_->base_offset);
        }

        if (element_index_) {
            message += " element #";
            message += std::to_string(*element_index_);
            if (element_total_) {
                message += " of ";
                message += std::to_string(*element_total_);
            }
        }

        if (offset_) {
            message += " at offset ";
            message += std::to_string(*offset_);
        }

        if (expected_size_ || actual_size_) {
            message += " (expected ";
            message += std::to_string(expected_size_.value_or(0));
            message += ", got ";
            message += std::to_string(actual_size_.value_or(0));
            message += ")";
        }
    }

    void append_source_location(std::string& message) const {
        if (!source_location_) {
            return;
        }

        message += " [created at ";
        message += source_location_file_name(source_location_->file_name());
        message += ':';
        message += std::to_string(source_location_->line());
        if (const char* function_name = source_location_->function_name(); function_name && *function_name) {
            message += " in ";
            message += function_name;
        }
        message += "]";
    }

    static std::string_view source_location_file_name(std::string_view file_name) noexcept {
        constexpr std::string_view repo_root = "/contract/";
        const auto pos = file_name.rfind(repo_root);
        if (pos != std::string_view::npos) {
            return file_name.substr(pos + 1);
        }
        const auto slash = file_name.find_last_of("/\\");
        if (slash == std::string_view::npos) {
            return file_name;
        }
        return file_name.substr(slash + 1);
    }

    Code code_{};
    std::optional<std::source_location> source_location_{};
    std::optional<std::string_view> type_name_{};
    std::optional<error_field> field_{};
    std::optional<Stage> stage_{};
    std::optional<std::size_t> offset_{};
    std::optional<std::size_t> element_index_{};
    std::optional<std::size_t> element_total_{};
    std::optional<std::size_t> expected_size_{};
    std::optional<std::size_t> actual_size_{};
};

} // namespace contract::detail
