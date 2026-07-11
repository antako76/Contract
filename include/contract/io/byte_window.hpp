#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/io/base.hpp>

#include <cstddef>
#include <cassert>
#include <span>
#include <string_view>

namespace contract::io {

class window_input {
public:
    explicit window_input(const std::byte* data, std::size_t size)
        : begin_(data)
        , current_(data)
        , end_(make_end(data, size)) {}

    explicit window_input(const unsigned char* data, std::size_t size)
        : window_input(reinterpret_cast<const std::byte*>(data), size) {}

    explicit window_input(std::string_view data)
        : window_input(reinterpret_cast<const std::byte*>(data.data()), data.size()) {}

    [[nodiscard]]
    std::span<const std::byte> peek(std::size_t max_size) const noexcept {
        if (max_size == 0 || current_ == end_) {
            return {};
        }

        const std::size_t available = static_cast<std::size_t>(end_ - current_);
        const std::size_t count = max_size < available ? max_size : available;
        return {current_, count};
    }

    void consume(std::size_t size) noexcept {
        if (size == 0) {
            return;
        }
        const std::size_t available = current_ == end_
            ? 0
            : static_cast<std::size_t>(end_ - current_);
        assert(size <= available);
        current_ += size;
    }

    // Diagnostic-only capability for error reporting. Not part of the hot parse path.
    [[nodiscard]]
    std::size_t position() const noexcept {
        return static_cast<std::size_t>(current_ - begin_);
    }

private:
    static const std::byte* make_end(const std::byte* data, std::size_t size) noexcept {
        if (size == 0) {
            return data;
        }
        assert(data != nullptr);
        return data + size;
    }

    const std::byte* begin_;
    const std::byte* current_;
    const std::byte* end_;
};

class window_output {
public:
    explicit window_output(std::byte* data, std::size_t size)
        : begin_(data)
        , current_(data)
        , end_(make_end(data, size)) {}

    explicit window_output(unsigned char* data, std::size_t size)
        : window_output(reinterpret_cast<std::byte*>(data), size) {}

    [[nodiscard]]
    std::span<std::byte> prepare(std::size_t max_size) noexcept {
        if (max_size == 0 || current_ == end_) {
            return {};
        }

        const std::size_t available = static_cast<std::size_t>(end_ - current_);
        const std::size_t count = max_size < available ? max_size : available;
        return {current_, count};
    }

    void commit(std::size_t size) noexcept {
        if (size == 0) {
            return;
        }
        const std::size_t available = current_ == end_
            ? 0
            : static_cast<std::size_t>(end_ - current_);
        assert(size <= available);
        current_ += size;
    }

    // Diagnostic-only capability for error reporting. Not part of the hot write path.
    [[nodiscard]]
    std::size_t position() const noexcept {
        return static_cast<std::size_t>(current_ - begin_);
    }

private:
    static std::byte* make_end(std::byte* data, std::size_t size) noexcept {
        if (size == 0) {
            return data;
        }
        assert(data != nullptr);
        return data + size;
    }

    std::byte* begin_;
    std::byte* current_;
    std::byte* end_;
};

} // namespace contract::io
