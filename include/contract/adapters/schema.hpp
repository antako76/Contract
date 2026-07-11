#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

#include <ostream>
#include <sstream>
#include <string>

namespace contract::adapters {

class schema_writer {
public:
    explicit schema_writer(std::ostream& out)
        : out_(out) {}

    template<class Field, class Value>
    void field(const Field& field, const Value&) {
        this->field(field);
    }

    template<class Field>
    void field(const Field& field) {
        if (!first_) {
            out_ << '\n';
        }

        first_ = false;
        out_ << Field::id << ' ' << field.name;
    }

private:
    std::ostream& out_;
    bool first_ = true;
};

template<class T>
std::string schema_string() {
    std::ostringstream out;
    schema_writer writer(out);

    contract::for_each_field<T>(
        [&](const auto&... fields) {
            (writer.field(fields), ...);
        });

    return out.str();
}

template<class Object>
std::string schema_string(const Object&) {
    return schema_string<Object>();
}

} // namespace contract::adapters
