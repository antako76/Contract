// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/io.hpp>

#include <cassert>
#include <cstddef>
#include <string_view>

#if defined(__has_include)
#  if __has_include(<contract/io/beast_window.hpp>) && __has_include(<boost/asio/buffer.hpp>)
#    include <contract/io/beast_window.hpp>
#  include <boost/asio/buffer.hpp>
#  endif
#endif

int main() {
    static_assert(contract::io::has_window_output<contract::io::window_output>);
    static_assert(contract::io::has_position<contract::io::window_output>);

    contract::io::string_view_input in("abcdef");

    const auto first = in.read_view(2);
    assert(first == "ab");

    char buffer[3]{};
    const auto read = in.read(buffer, 3);
    assert(read == 3);
    assert(std::string_view(buffer, read) == "cde");

    const auto tail = in.read_view(10);
    assert(tail == "f");

    const auto empty = in.read_view(1);
    assert(empty.empty());

    unsigned char out_storage[6]{};
    contract::io::window_output out(out_storage, sizeof(out_storage));

    auto first_window = out.prepare(4);
    assert(first_window.size() == 4);
    first_window[0] = std::byte{'a'};
    first_window[1] = std::byte{'b'};
    first_window[2] = std::byte{'c'};
    out.commit(3);
    assert(out.position() == 3);

    auto tail_window = out.prepare(10);
    assert(tail_window.size() == 3);
    tail_window[0] = std::byte{'d'};
    tail_window[1] = std::byte{'e'};
    tail_window[2] = std::byte{'f'};
    out.commit(tail_window.size());
    assert(out.position() == sizeof(out_storage));
    assert(out.prepare(1).empty());

    assert(out_storage[0] == 'a');
    assert(out_storage[1] == 'b');
    assert(out_storage[2] == 'c');
    assert(out_storage[3] == 'd');
    assert(out_storage[4] == 'e');
    assert(out_storage[5] == 'f');

#if defined(__has_include) && __has_include(<contract/io/beast_window.hpp>) && __has_include(<boost/asio/buffer.hpp>)
    {
        const unsigned char bytes[] = {'a', 'b', 'c', 'd', 'e', 'f'};
        boost::beast::flat_buffer buffer;
        const auto writable = buffer.prepare(sizeof(bytes));
        const auto copied = boost::asio::buffer_copy(writable, boost::asio::buffer(bytes));
        assert(copied == sizeof(bytes));
        buffer.commit(copied);

        contract::io::flat_buffer_input window{buffer};

        const auto first = window.peek(3);
        assert(first.size() == 3);
        assert(std::to_integer<unsigned char>(first[0]) == 'a');
        assert(std::to_integer<unsigned char>(first[1]) == 'b');
        assert(std::to_integer<unsigned char>(first[2]) == 'c');

        window.consume(2);

        const auto tail = window.peek(10);
        assert(tail.size() == 4);
        assert(std::to_integer<unsigned char>(tail[0]) == 'c');
        assert(std::to_integer<unsigned char>(tail[3]) == 'f');
    }
#endif

    return 0;
}
