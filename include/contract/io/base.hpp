#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <concepts>

namespace contract::io {

template<class T>
concept has_read_view = requires(T& input, std::size_t size) {
    input.read_view(size);
};

template<class T>
concept has_window_input = requires(T& input, std::size_t size) {
    input.peek(size);
    input.consume(size);
};

template<class T>
concept has_write = requires(T& output, const void* data, std::size_t size) {
    output.write(data, size);
};

template<class T>
concept has_window_output = requires(T& output, std::size_t size) {
    output.prepare(size);
    output.commit(size);
};

template<class T>
concept has_position = requires(const T& input) {
    { input.position() } -> std::convertible_to<std::size_t>;
};

} // namespace contract::io
