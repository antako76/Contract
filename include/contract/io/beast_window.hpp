#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#if defined(__has_include)
#  if __has_include(<boost/beast/core/flat_buffer.hpp>) && __has_include(<boost/asio/buffer.hpp>)

#    include <boost/asio/buffer.hpp>
#    include <boost/beast/core/flat_buffer.hpp>

#    include <cstddef>
#    include <cassert>
#    include <span>

namespace contract::io {

class flat_buffer_input {
public:
    explicit flat_buffer_input(boost::beast::flat_buffer& buffer) noexcept
        : buffer_(buffer) {}

    [[nodiscard]]
    std::span<const std::byte> peek(std::size_t max_size) const noexcept {
        const std::size_t available = buffer_.size();
        if (max_size == 0 || available == 0) {
            return {};
        }

        const std::size_t count = max_size < available ? max_size : available;
        const auto readable = buffer_.data();
        const auto first = boost::asio::buffer_sequence_begin(readable);

        const auto* bytes = boost::asio::buffer_cast<const unsigned char*>(*first);
        return {reinterpret_cast<const std::byte*>(bytes), count};
    }

    void consume(std::size_t size) noexcept {
        if (size == 0) {
            return;
        }
        assert(size <= buffer_.size());
        buffer_.consume(size);
    }

private:
    boost::beast::flat_buffer& buffer_;
};

} // namespace contract::io

#  endif
#endif
