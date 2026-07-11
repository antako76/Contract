#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cassert>
#include <cstring>
#include <string_view>

namespace contract::io {

class string_view_input {
public:
    explicit string_view_input(std::string_view input)
        : input_(input) {}

    [[nodiscard]]
    std::string_view read_view(std::size_t size) {
        if (size == 0 || pos_ >= input_.size()) {
            return {};
        }

        const std::size_t available = input_.size() - pos_;
        const std::size_t count = size < available ? size : available;
        const auto view = input_.substr(pos_, count);
        pos_ += count;
        return view;
    }

    std::size_t read(void* out, std::size_t size) {
        if (size == 0 || pos_ >= input_.size()) {
            return 0;
        }
        if (out == nullptr) {
            assert(out != nullptr);
            return 0;
        }

        const auto view = read_view(size);
        const std::size_t count = view.size();
        std::memcpy(out, view.data(), count);
        return count;
    }

private:
    std::string_view input_;
    std::size_t pos_ = 0;
};

} // namespace contract::io
