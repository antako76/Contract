#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cassert>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace contract::io {

class file_input {
public:
    explicit file_input(std::string path)
        : path_(std::move(path))
        , stream_(path_, std::ios::binary) {
        if (!stream_) {
            throw std::runtime_error("contract io file_input: failed to open file: " + path_);
        }
    }

    std::size_t read(void* out, std::size_t size) {
        if (size == 0) {
            return 0;
        }
        if (out == nullptr) {
            assert(out != nullptr);
            return 0;
        }

        stream_.read(static_cast<char*>(out), static_cast<std::streamsize>(size));
        return static_cast<std::size_t>(stream_.gcount());
    }

private:
    std::string path_;
    std::ifstream stream_;
};

class file_buffer_input {
public:
    explicit file_buffer_input(std::string path)
        : storage_(read_file(path)) {}

    [[nodiscard]]
    std::span<const std::byte> peek(std::size_t max_size) const noexcept {
        if (max_size == 0 || pos_ >= storage_.size()) {
            return {};
        }

        const std::size_t available = storage_.size() - pos_;
        const std::size_t count = max_size < available ? max_size : available;
        return {reinterpret_cast<const std::byte*>(storage_.data() + pos_), count};
    }

    void consume(std::size_t size) noexcept {
        if (size == 0) {
            return;
        }
        assert(size <= storage_.size() - pos_);
        pos_ += size;
    }

    [[nodiscard]]
    std::size_t position() const noexcept {
        return pos_;
    }

private:
    static std::string read_file(const std::string& path) {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            throw std::runtime_error("contract io file_buffer_input: failed to open file: " + path);
        }

        return {
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()};
    }

    std::string storage_;
    std::size_t pos_ = 0;
};

} // namespace contract::io
