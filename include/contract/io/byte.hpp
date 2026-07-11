#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/io/base.hpp>

#include <cstddef>
#include <cassert>
#include <cstring>

namespace contract::io {

class output {
public:
    explicit output(unsigned char* data)
        : current_(data) {}

    std::size_t write(const void* data, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (data == nullptr) {
            assert(data != nullptr);
            return 0;
        }
        std::memcpy(current_, data, size);
        current_ += size;
        return size;
    }

    [[nodiscard]]
    unsigned char* current() const {
        return current_;
    }

private:
    unsigned char* current_;
};

class checked_output {
public:
    checked_output(unsigned char* data, std::size_t size)
        : current_(data)
        , end_(make_end(data, size)) {}

    std::size_t write(const void* data, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (data == nullptr) {
            assert(data != nullptr);
            return 0;
        }
        const auto available = available_size();
        if (size > available) {
            return 0;
        }
        std::memcpy(current_, data, size);
        current_ += size;
        return size;
    }

    [[nodiscard]]
    unsigned char* current() const {
        return current_;
    }

private:
    static unsigned char* make_end(unsigned char* data, std::size_t size) {
        if (size == 0) {
            return data;
        }
        assert(data != nullptr);
        return data + size;
    }

    std::size_t available_size() const noexcept {
        if (current_ == nullptr || end_ == nullptr) {
            return 0;
        }
        return static_cast<std::size_t>(end_ - current_);
    }

    unsigned char* current_;
    unsigned char* end_;
};

class input {
public:
    explicit input(const unsigned char* data)
        : current_(data) {}

    [[nodiscard]]
    const unsigned char* read_view(std::size_t size) {
        if (size == 0) {
            return current_;
        }
        const unsigned char* data = current_;
        current_ += size;
        return data;
    }

    std::size_t read(void* out, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (out == nullptr) {
            assert(out != nullptr);
            return 0;
        }
        const unsigned char* data = read_view(size);
        std::memcpy(out, data, size);
        return size;
    }

    [[nodiscard]]
    const unsigned char* current() const {
        return current_;
    }

private:
    const unsigned char* current_;
};

class checked_input {
public:
    checked_input(const unsigned char* data, std::size_t size)
        : current_(data)
        , end_(make_end(data, size)) {}

    [[nodiscard]]
    const unsigned char* read_view(std::size_t size) {
        if (size == 0) {
            return current_;
        }
        if (size > available_size()) {
            return nullptr;
        }
        const unsigned char* data = current_;
        current_ += size;
        return data;
    }

    std::size_t read(void* out, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (out == nullptr) {
            assert(out != nullptr);
            return 0;
        }
        const unsigned char* data = read_view(size);
        if (data == nullptr) {
            return 0;
        }
        std::memcpy(out, data, size);
        return size;
    }

    [[nodiscard]]
    const unsigned char* current() const {
        return current_;
    }

private:
    static const unsigned char* make_end(const unsigned char* data, std::size_t size) {
        if (size == 0) {
            return data;
        }
        assert(data != nullptr);
        return data + size;
    }

    std::size_t available_size() const noexcept {
        if (current_ == nullptr || end_ == nullptr) {
            return 0;
        }
        return static_cast<std::size_t>(end_ - current_);
    }

    const unsigned char* current_;
    const unsigned char* end_;
};

} // namespace contract::io
