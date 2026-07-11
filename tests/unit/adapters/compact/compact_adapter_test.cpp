// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/compact/all.hpp>
#include <contract/check.hpp>
#include <contract/contract.hpp>
#include <contract/io/byte.hpp>

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <span>
#include <tuple>
#include <variant>
#include <unordered_map>
#include <vector>

namespace {

using contract::adapters::base::status;
using contract::adapters::compact::reader;
using contract::adapters::compact::value_kind;
using contract::adapters::compact::writer;

enum class SampleEnum : std::uint16_t {
    value = 300,
};

struct CompactChild {
    std::uint32_t value = 7;

    CONTRACT(CompactChild, (value, 1))
};

struct CompactRecord {
    std::uint32_t id = 42;
    std::string name = "neo";
    CompactChild child{};

    CONTRACT(CompactRecord, (name, 2), (child, 3), (id, 1))
};

struct CompactOptionalRecord {
    std::optional<std::uint32_t> count{17};
    std::optional<std::string> name{};
    std::optional<CompactChild> child{CompactChild{9}};

    CONTRACT(CompactOptionalRecord, (count, 1), (name, 2), (child, 3))
};

struct CompactVectorRecord {
    std::vector<std::uint32_t> values{1, 64, 2};
    std::vector<CompactChild> children{CompactChild{5}, CompactChild{6}};

    CONTRACT(CompactVectorRecord, (values, 1), (children, 2))
};

struct CompactArrayRecord {
    std::array<std::uint32_t, 3> values{1, 64, 2};
    std::array<CompactChild, 2> children{CompactChild{5}, CompactChild{6}};

    CONTRACT(CompactArrayRecord, (values, 1), (children, 2))
};

struct CompactFixedCharArrayRecord {
    std::uint64_t id = 31;
    char code[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};

    CONTRACT(CompactFixedCharArrayRecord, (id, 1), (code, 2))
};

struct CompactFixedUIntArrayRecord {
    std::uint32_t values[4] = {1, 2, 3, 4};

    CONTRACT(CompactFixedUIntArrayRecord, (values, 1))
};

struct CompactByteStdArrayRecord {
    std::array<unsigned char, 4> data{0x11, 0x22, 0x33, 0x44};

    CONTRACT(CompactByteStdArrayRecord, (data, 1))
};

struct CompactMapRecord {
    std::map<std::uint32_t, std::string> labels{{1, "one"}, {2, "two"}};
    std::unordered_map<std::uint32_t, CompactChild> children{{3, CompactChild{7}}, {4, CompactChild{8}}};

    CONTRACT(CompactMapRecord, (labels, 1), (children, 2))
};

struct CompactTupleVariantRecord {
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 2>> tuple_value{
        1, "hi", {5, 6}
    };
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 2>> variant_value{
        std::array<std::uint32_t, 2>{7, 8}
    };

    CONTRACT(CompactTupleVariantRecord, (tuple_value, 1), (variant_value, 2))
};

void assert_bytes(const unsigned char* actual, const unsigned char* expected, std::size_t size);

void test_span_codec() {
    {
        std::array<std::byte, 4> source_bytes{
            std::byte{0x11},
            std::byte{0x22},
            std::byte{0x33},
            std::byte{0x44},
        };
        std::array<std::byte, 4> target_bytes{};

        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        assert(out.write_value(std::span<const std::byte>{source_bytes}) == status::ok);

        const unsigned char expected[] = {
            0x64,
            0x11,
            0x22,
            0x33,
            0x44,
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        reader<> in{buffer.data(), sizeof(expected)};
        auto target_span = std::span<std::byte>{target_bytes};
        assert(in.read_value(target_span) == status::ok);
        assert(target_bytes == source_bytes);
    }

    {
        std::array<std::uint32_t, 3> source_values{1, 64, 2};
        std::array<std::uint32_t, 3> target_values{};

        std::array<unsigned char, 64> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        assert(out.write_value(std::span<const std::uint32_t>{source_values}) == status::ok);

        reader<> in{buffer.data(), buffer.size()};
        auto target_span = std::span<std::uint32_t>{target_values};
        assert(in.read_value(target_span) == status::ok);
        assert(target_values == source_values);
    }
}

void test_bitset_codec() {
    std::bitset<10> source{0b1010010110};
    std::bitset<10> target{};

    std::array<unsigned char, 32> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
    assert(out.write_value(source) == status::ok);

    const unsigned char expected[] = {
        0x62,
        0x96,
        0x02,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), sizeof(expected)};
    assert(in.read_value(target) == status::ok);
    assert(target == source);
}

struct CompactLimitedVectorRecord {
    std::vector<std::uint32_t> values{};

    CONTRACT(CompactLimitedVectorRecord, (values, 1, contract::check::max_items(2)))
};

struct CompactLimitedArrayRecord {
    std::array<std::uint32_t, 3> values{1, 2, 3};

    CONTRACT(CompactLimitedArrayRecord, (values, 1, contract::check::max_items(2)))
};

struct CompactLimitedMapRecord {
    std::map<std::uint32_t, std::string> labels{};

    CONTRACT(CompactLimitedMapRecord, (labels, 1, contract::check::max_items(1)))
};

void assert_bytes(const unsigned char* actual, const unsigned char* expected, std::size_t size) {
    assert(std::memcmp(actual, expected, size) == 0);
}

void test_integer_headers() {
    std::array<unsigned char, 32> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    assert(out.write_uint(0) == status::ok);
    assert(out.write_uint(42) == status::ok);
    assert(out.write_int(-1) == status::ok);
    assert(out.write_int(-16) == status::ok);
    assert(out.write_uint(64) == status::ok);
    assert(out.write_uint(256) == status::ok);
    assert(out.write_int(-17) == status::ok);
    assert(out.write_int(-256) == status::ok);

    const unsigned char expected[] = {
        0x00,
        0x2a,
        0x40,
        0x4f,
        0x50, 0x40,
        0x51, 0x00, 0x01,
        0x58, 0x11,
        0x59, 0x00, 0x01,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    std::uint64_t unsigned_value = 0;
    std::int64_t signed_value = 0;

    assert(in.read_uint(unsigned_value) == status::ok);
    assert(unsigned_value == 0);
    assert(in.read_uint(unsigned_value) == status::ok);
    assert(unsigned_value == 42);
    assert(in.read_int(signed_value) == status::ok);
    assert(signed_value == -1);
    assert(in.read_int(signed_value) == status::ok);
    assert(signed_value == -16);
    assert(in.read_uint(unsigned_value) == status::ok);
    assert(unsigned_value == 64);
    assert(in.read_uint(unsigned_value) == status::ok);
    assert(unsigned_value == 256);
    assert(in.read_int(signed_value) == status::ok);
    assert(signed_value == -17);
    assert(in.read_int(signed_value) == status::ok);
    assert(signed_value == -256);
}

void test_size_headers() {
    std::array<unsigned char, 16> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    assert(out.write_size_header(value_kind::string, 5) == status::ok);
    assert(out.write_size_header(value_kind::string, 100) == status::ok);
    assert(out.write_size_header(value_kind::object, 3) == status::ok);
    assert(out.write_size_header(value_kind::object, 20) == status::ok);

    const unsigned char expected[] = {
        0x75,
        0x7f, 0x50, 0x64,
        0xa3,
        0xaf, 0x14,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    std::uint64_t size = 0;
    assert(in.read_size_header(value_kind::string, size) == status::ok);
    assert(size == 5);
    assert(in.read_size_header(value_kind::string, size) == status::ok);
    assert(size == 100);
    assert(in.read_size_header(value_kind::object, size) == status::ok);
    assert(size == 3);
    assert(in.read_size_header(value_kind::object, size) == status::ok);
    assert(size == 20);
}

void test_bool_null_and_float() {
    std::array<unsigned char, 32> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    assert(out.write_bool(false) == status::ok);
    assert(out.write_bool(true) == status::ok);
    assert(out.write_null() == status::ok);
    assert(out.write_float(1.5f) == status::ok);
    assert(out.write_float(2.25) == status::ok);

    const unsigned char expected_prefix[] = {
        0xb0,
        0xb1,
        0xb2,
        0xc4, 0x00, 0x00, 0xc0, 0x3f,
        0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40,
    };
    assert_bytes(buffer.data(), expected_prefix, sizeof(expected_prefix));

    reader<> in{buffer.data(), buffer.size()};
    bool flag = true;
    assert(in.read_bool(flag) == status::ok);
    assert(!flag);
    assert(in.read_bool(flag) == status::ok);
    assert(flag);
    assert(in.read_null() == status::ok);

    float f = 0.0f;
    double d = 0.0;
    assert(in.read_float(f) == status::ok);
    assert(f == 1.5f);
    assert(in.read_float(d) == status::ok);
    assert(d == 2.25);
}

void test_skip_value() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    assert(out.write_size_header(value_kind::object, 2) == status::ok);
    assert(out.write_uint(7) == status::ok);
    assert(out.write_size_header(value_kind::string, 3) == status::ok);
    assert(out.write("abc", 3) == status::ok);
    assert(out.write_uint(9) == status::ok);
    assert(out.write_size_header(value_kind::array, 2) == status::ok);
    assert(out.write_int(-1) == status::ok);
    assert(out.write_bool(true) == status::ok);
    assert(out.write_uint(42) == status::ok);

    reader<> in{buffer.data(), buffer.size()};
    assert(in.skip_value() == status::ok);

    std::uint64_t value = 0;
    assert(in.read_uint(value) == status::ok);
    assert(value == 42);
}

void test_value_codecs() {
    std::array<unsigned char, 128> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::string text = "hello";
    const std::string_view view = "view";

    out << true;
    out << std::int32_t{-300};
    out << std::uint64_t{1000};
    out << SampleEnum::value;
    out << 1.25f;
    out << 2.5;
    out << text;
    out << view;

    reader<> in{buffer.data(), buffer.size()};
    bool flag = false;
    std::int32_t signed_value = 0;
    std::uint64_t unsigned_value = 0;
    SampleEnum enum_value{};
    float float_value = 0.0f;
    double double_value = 0.0;
    std::string decoded_text;
    std::string_view decoded_view;

    in >> flag;
    in >> signed_value;
    in >> unsigned_value;
    in >> enum_value;
    in >> float_value;
    in >> double_value;
    in >> decoded_text;
    in >> decoded_view;

    assert(flag);
    assert(signed_value == -300);
    assert(unsigned_value == 1000);
    assert(enum_value == SampleEnum::value);
    assert(float_value == 1.25f);
    assert(double_value == 2.5);
    assert(decoded_text == text);
    assert(decoded_view == view);
}

void test_optional_codecs() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::optional<std::uint32_t> present{300};
    const std::optional<std::uint32_t> empty{};
    const std::optional<std::string> text{"opt"};
    const std::optional<CompactChild> child{CompactChild{11}};

    out << present;
    out << empty;
    out << text;
    out << child;

    const unsigned char expected[] = {
        0x51, 0x2c, 0x01,
        0xb2,
        0x73, 'o', 'p', 't',
        0xa1, 0x01, 0x0b,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    std::optional<std::uint32_t> decoded_present;
    std::optional<std::uint32_t> decoded_empty{7};
    std::optional<std::string> decoded_text;
    std::optional<CompactChild> decoded_child;

    in >> decoded_present;
    in >> decoded_empty;
    in >> decoded_text;
    in >> decoded_child;

    assert(decoded_present == present);
    assert(!decoded_empty.has_value());
    assert(decoded_text == text);
    assert(decoded_child.has_value());
    assert(decoded_child->value == child->value);
}

void test_vector_codecs() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::vector<std::uint32_t> numbers{1, 64, 2};
    const std::vector<std::string> words{"a", "bc"};

    out << numbers;
    out << words;

    const unsigned char expected[] = {
        0x83, 0x01, 0x50, 0x40, 0x02,
        0x82, 0x71, 'a', 0x72, 'b', 'c',
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    std::vector<std::uint32_t> decoded_numbers;
    std::vector<std::string> decoded_words;

    in >> decoded_numbers;
    in >> decoded_words;

    assert(decoded_numbers == numbers);
    assert(decoded_words == words);
}

void test_array_codecs() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::array<std::uint32_t, 3> numbers{1, 64, 2};
    const std::array<std::string, 2> words{"a", "bc"};

    out << numbers;
    out << words;

    const unsigned char expected[] = {
        0x83, 0x01, 0x50, 0x40, 0x02,
        0x82, 0x71, 'a', 0x72, 'b', 'c',
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    std::array<std::uint32_t, 3> decoded_numbers{};
    std::array<std::string, 2> decoded_words{};

    in >> decoded_numbers;
    in >> decoded_words;

    assert(decoded_numbers == numbers);
    assert(decoded_words == words);
}

void test_fixed_array_codecs() {
    {
        // char[N]: byte-like element -> single bytes-kind blob, not per-element.
        // Fully packed (no trailing zero) -> nothing to trim, all N bytes written.
        char source[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};
        char target[8] = {};

        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        out << source;

        const unsigned char expected[] = {
            0x68, 'c', 'o', 'n', 't', 'r', 'a', 'c', 't',
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;
        assert(std::memcmp(source, target, sizeof(source)) == 0);
    }

    {
        // char[N] with trailing zero padding -> trimmed on write; the reader
        // must zero-fill the tail, even over stale/garbage target contents.
        char source[16] = {'h', 'i'};
        char target[16];
        std::memset(target, 0xee, sizeof(target));

        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        out << source;

        const unsigned char expected[] = {
            0x62, 'h', 'i',
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;
        assert(std::memcmp(source, target, sizeof(source)) == 0);
    }

    {
        // uint32_t[N]: not byte-like -> array-kind header + per-element compact ints.
        std::uint32_t source[4] = {1, 2, 3, 4};
        std::uint32_t target[4] = {};

        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        out << source;

        const unsigned char expected[] = {
            0x84, 0x01, 0x02, 0x03, 0x04,
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;
        assert(std::memcmp(source, target, sizeof(source)) == 0);
    }

    {
        // std::array<unsigned char, N> also takes the bulk bytes-kind path.
        std::array<unsigned char, 4> source{0x11, 0x22, 0x33, 0x44};
        std::array<unsigned char, 4> target{};

        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};
        out << source;

        const unsigned char expected[] = {
            0x64, 0x11, 0x22, 0x33, 0x44,
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;
        assert(target == source);
    }
}

void test_map_codecs() {
    std::array<unsigned char, 128> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::map<std::uint32_t, std::string> ordered{{1, "one"}, {2, "two"}};
    const std::unordered_map<std::uint32_t, std::uint32_t> unordered{{3, 30}, {4, 40}};

    out << ordered;
    out << unordered;

    const unsigned char expected_prefix[] = {
        0x92,
        0x01, 0x73, 'o', 'n', 'e',
        0x02, 0x73, 't', 'w', 'o',
    };
    assert_bytes(buffer.data(), expected_prefix, sizeof(expected_prefix));

    reader<> in{buffer.data(), buffer.size()};
    std::map<std::uint32_t, std::string> decoded_ordered;
    std::unordered_map<std::uint32_t, std::uint32_t> decoded_unordered;

    in >> decoded_ordered;
    in >> decoded_unordered;

    assert(decoded_ordered == ordered);
    assert(decoded_unordered == unordered);
}

void test_tuple_codecs() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 2>> value{
        1, "hi", std::array<std::uint32_t, 2>{5, 6}
    };

    out << value;

    const unsigned char expected[] = {
        0x83,
        0x01,
        0x72, 'h', 'i',
        0x82, 0x05, 0x06,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    using tuple_type = std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 2>>;
    tuple_type decoded{};
    in >> decoded;

    assert(decoded == value);
}

void test_variant_codecs() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    const std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 2>> value{
        std::array<std::uint32_t, 2>{7, 8}
    };

    out << value;

    const unsigned char expected[] = {
        0x82,
        0x02,
        0x82, 0x07, 0x08,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    reader<> in{buffer.data(), buffer.size()};
    using variant_type = std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 2>>;
    variant_type decoded{};
    in >> decoded;

    assert(decoded == value);
}

void test_contract_object_roundtrip() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactRecord source{};
    out << source;

    const unsigned char expected[] = {
        0xa3,
        0x02, 0x73, 'n', 'e', 'o',
        0x03, 0xa1, 0x01, 0x07,
        0x01, 0x2a,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    CompactRecord target{};
    target.id = 0;
    target.name = {};
    target.child.value = 0;

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.id == source.id);
    assert(target.name == source.name);
    assert(target.child.value == source.child.value);
}

void test_contract_object_vector_fields() {
    std::array<unsigned char, 128> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactVectorRecord source{};
    out << source;

    const unsigned char expected[] = {
        0xa2,
        0x01, 0x83, 0x01, 0x50, 0x40, 0x02,
        0x02, 0x82, 0xa1, 0x01, 0x05, 0xa1, 0x01, 0x06,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    CompactVectorRecord target{};
    target.values.clear();
    target.children.clear();

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.values == source.values);
    assert(target.children.size() == source.children.size());
    assert(target.children[0].value == 5);
    assert(target.children[1].value == 6);
}

void test_contract_object_array_fields() {
    std::array<unsigned char, 128> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactArrayRecord source{};
    out << source;

    const unsigned char expected[] = {
        0xa2,
        0x01, 0x83, 0x01, 0x50, 0x40, 0x02,
        0x02, 0x82, 0xa1, 0x01, 0x05, 0xa1, 0x01, 0x06,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    CompactArrayRecord target{};
    target.values = {};
    target.children = {};

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.values == source.values);
    assert(target.children[0].value == 5);
    assert(target.children[1].value == 6);
}

void test_contract_object_fixed_array_fields() {
    {
        std::array<unsigned char, 64> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

        CompactFixedCharArrayRecord source{};
        out << source;

        const unsigned char expected[] = {
            0xa2,
            0x01, 0x1f,
            0x02, 0x68, 'c', 'o', 'n', 't', 'r', 'a', 'c', 't',
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        CompactFixedCharArrayRecord target{};
        std::memset(target.code, 0, sizeof(target.code));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;

        assert(target.id == source.id);
        assert(std::memcmp(target.code, source.code, sizeof(source.code)) == 0);
    }

    {
        std::array<unsigned char, 64> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

        CompactFixedUIntArrayRecord source{};
        out << source;

        const unsigned char expected[] = {
            0xa1,
            0x01, 0x84, 0x01, 0x02, 0x03, 0x04,
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        CompactFixedUIntArrayRecord target{};
        std::memset(target.values, 0, sizeof(target.values));

        reader<> in{buffer.data(), buffer.size()};
        in >> target;

        assert(std::memcmp(target.values, source.values, sizeof(source.values)) == 0);
    }

    {
        std::array<unsigned char, 32> buffer{};
        writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

        CompactByteStdArrayRecord source{};
        out << source;

        const unsigned char expected[] = {
            0xa1,
            0x01, 0x64, 0x11, 0x22, 0x33, 0x44,
        };
        assert_bytes(buffer.data(), expected, sizeof(expected));

        CompactByteStdArrayRecord target{};
        target.data = {};

        reader<> in{buffer.data(), buffer.size()};
        in >> target;

        assert(target.data == source.data);
    }
}

void test_contract_object_map_fields() {
    std::array<unsigned char, 256> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactMapRecord source{};
    out << source;

    const unsigned char expected_prefix[] = {
        0xa2,
        0x01, 0x92,
        0x01, 0x73, 'o', 'n', 'e',
        0x02, 0x73, 't', 'w', 'o',
        0x02, 0x92,
    };
    assert_bytes(buffer.data(), expected_prefix, sizeof(expected_prefix));

    CompactMapRecord target{};
    target.labels.clear();
    target.children.clear();

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.labels == source.labels);
    assert(target.children.size() == source.children.size());
    assert(target.children.at(3).value == 7);
    assert(target.children.at(4).value == 8);
}

void test_contract_object_tuple_variant_fields() {
    std::array<unsigned char, 128> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactTupleVariantRecord source{};
    out << source;

    const unsigned char expected[] = {
        0xa2,
        0x01, 0x83, 0x01, 0x72, 'h', 'i', 0x82, 0x05, 0x06,
        0x02, 0x82, 0x02, 0x82, 0x07, 0x08,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    CompactTupleVariantRecord target{};
    target.tuple_value = {};
    target.variant_value = std::uint32_t{0};

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.tuple_value == source.tuple_value);
    assert(target.variant_value == source.variant_value);
}

void test_contract_object_missing_and_unknown_fields() {
    const unsigned char data[] = {
        0xa3,
        0x09, 0x74, 's', 'k', 'i', 'p',
        0x01, 0x11,
        0x08, 0xa1, 0x01, 0x0f,
    };

    CompactRecord target{};
    target.id = 1;
    target.name = "keep";
    target.child.value = 2;

    reader<> in{data, sizeof(data)};
    in >> target;

    assert(target.id == 17);
    assert(target.name == "keep");
    assert(target.child.value == 2);
}

void test_contract_object_vector_max_items_error() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactLimitedVectorRecord source{};
    source.values = {1, 2, 3};

    bool write_thrown = false;
    try {
        out << source;
    } catch (const std::runtime_error& error) {
        write_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("values") != std::string::npos);
    }
    assert(write_thrown);

    const unsigned char data[] = {
        0xa1,
        0x01, 0x83, 0x01, 0x02, 0x03,
    };
    CompactLimitedVectorRecord target{};
    reader<> in{data, sizeof(data)};

    bool read_thrown = false;
    try {
        in >> target;
    } catch (const std::runtime_error& error) {
        read_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("values") != std::string::npos);
    }
    assert(read_thrown);
}

void test_contract_object_array_errors() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactLimitedArrayRecord source{};

    bool write_thrown = false;
    try {
        out << source;
    } catch (const std::runtime_error& error) {
        write_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("values") != std::string::npos);
    }
    assert(write_thrown);

    const unsigned char limited_data[] = {
        0xa1,
        0x01, 0x83, 0x01, 0x02, 0x03,
    };
    CompactLimitedArrayRecord limited_target{};
    reader<> limited_in{limited_data, sizeof(limited_data)};

    bool limit_read_thrown = false;
    try {
        limited_in >> limited_target;
    } catch (const std::runtime_error& error) {
        limit_read_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("values") != std::string::npos);
    }
    assert(limit_read_thrown);

    const unsigned char wrong_size_data[] = {
        0x82, 0x01, 0x02,
    };
    std::array<std::uint32_t, 3> target{};
    reader<> wrong_size_in{wrong_size_data, sizeof(wrong_size_data)};

    bool size_read_thrown = false;
    try {
        wrong_size_in >> target;
    } catch (const std::runtime_error& error) {
        size_read_thrown = true;
        const std::string message = error.what();
        assert(message.find("invalid size") != std::string::npos);
    }
    assert(size_read_thrown);
}

void test_contract_object_map_errors() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactLimitedMapRecord source{};
    source.labels = {{1, "one"}, {2, "two"}};

    bool write_thrown = false;
    try {
        out << source;
    } catch (const std::runtime_error& error) {
        write_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("labels") != std::string::npos);
    }
    assert(write_thrown);

    const unsigned char limited_data[] = {
        0xa1,
        0x01, 0x92,
        0x01, 0x71, 'a',
        0x02, 0x71, 'b',
    };
    CompactLimitedMapRecord limited_target{};
    reader<> limited_in{limited_data, sizeof(limited_data)};

    bool limit_read_thrown = false;
    try {
        limited_in >> limited_target;
    } catch (const std::runtime_error& error) {
        limit_read_thrown = true;
        const std::string message = error.what();
        assert(message.find("max items exceeded") != std::string::npos);
        assert(message.find("labels") != std::string::npos);
    }
    assert(limit_read_thrown);

    const unsigned char duplicate_data[] = {
        0x92,
        0x01, 0x71, 'a',
        0x01, 0x71, 'b',
    };
    std::map<std::uint32_t, std::string> target{};
    reader<> duplicate_in{duplicate_data, sizeof(duplicate_data)};

    bool duplicate_read_thrown = false;
    try {
        duplicate_in >> target;
    } catch (const std::runtime_error& error) {
        duplicate_read_thrown = true;
        const std::string message = error.what();
        assert(message.find("duplicate key") != std::string::npos);
    }
    assert(duplicate_read_thrown);
}

void test_contract_object_tuple_variant_errors() {
    const unsigned char tuple_wrong_size[] = {
        0x82, 0x01, 0x71, 'x'
    };
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 2>> tuple_target{};
    reader<> tuple_in{tuple_wrong_size, sizeof(tuple_wrong_size)};

    bool tuple_thrown = false;
    try {
        tuple_in >> tuple_target;
    } catch (const std::runtime_error& error) {
        tuple_thrown = true;
        const std::string message = error.what();
        assert(message.find("invalid size") != std::string::npos);
    }
    assert(tuple_thrown);

    const unsigned char variant_bad_index[] = {
        0x82, 0x05,
    };
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 2>> variant_target{};
    reader<> variant_in{variant_bad_index, sizeof(variant_bad_index)};

    bool variant_thrown = false;
    try {
        variant_in >> variant_target;
    } catch (const std::runtime_error& error) {
        variant_thrown = true;
        const std::string message = error.what();
        assert(message.find("variant index out of range") != std::string::npos);
    }
    assert(variant_thrown);
}

void test_contract_object_out_of_order_fields() {
    const unsigned char data[] = {
        0xa3,
        0x03, 0xa1, 0x01, 0x09,
        0x02, 0x73, 'b', 'o', 'b',
        0x01, 0x2a,
    };

    CompactRecord target{};
    target.id = 0;
    target.name = {};
    target.child.value = 0;

    reader<> in{data, sizeof(data)};
    in >> target;

    assert(target.id == 42);
    assert(target.name == "bob");
    assert(target.child.value == 9);
}

void test_contract_object_duplicate_field_last_wins() {
    const unsigned char data[] = {
        0xa2,
        0x01, 0x01,
        0x01, 0x02,
    };

    CompactRecord target{};
    reader<> in{data, sizeof(data)};
    in >> target;

    assert(target.id == 2);
}

void test_contract_object_optional_fields() {
    std::array<unsigned char, 64> buffer{};
    writer<> out{contract::io::window_output{buffer.data(), buffer.size()}};

    CompactOptionalRecord source{};
    out << source;

    const unsigned char expected[] = {
        0xa3,
        0x01, 0x11,
        0x02, 0xb2,
        0x03, 0xa1, 0x01, 0x09,
    };
    assert_bytes(buffer.data(), expected, sizeof(expected));

    CompactOptionalRecord target{};
    target.count.reset();
    target.name = "old";
    target.child.reset();

    reader<> in{buffer.data(), buffer.size()};
    in >> target;

    assert(target.count == source.count);
    assert(!target.name.has_value());
    assert(target.child.has_value());
    assert(target.child->value == source.child->value);
}

void test_checked_io_errors() {
    unsigned char buffer[1]{};
    writer<> out{contract::io::window_output{buffer, sizeof(buffer)}};
    assert(out.write_uint(256) == status::error);
    assert(static_cast<const writer<>&>(out).error().has_value());

    const unsigned char truncated[] = {0x51, 0x00};
    reader<> in{truncated, sizeof(truncated)};
    std::uint64_t value = 0;
    assert(in.read_uint(value) == status::error);
    assert(static_cast<const reader<>&>(in).error().has_value());
}

// Robustness against truncated/malformed/adversarial input: the reader must
// return an error status, never over-read the window or crash.
void test_truncated_input_errors() {
    {
        // int_payload header claiming 2 payload bytes, none present.
        const unsigned char data[] = {0x51};
        reader<> in{data, sizeof(data)};
        std::uint64_t v = 0;
        assert(in.read_uint(v) == status::error);
    }
    {
        // string inline size 5, no payload bytes.
        const unsigned char data[] = {0x75};
        reader<> in{data, sizeof(data)};
        std::string s;
        assert(in.read_value(s) == status::error);
    }
    {
        // float32 header with only 1 of 4 payload bytes.
        const unsigned char data[] = {0xc4, 0x00};
        reader<> in{data, sizeof(data)};
        float f = 0.0f;
        assert(in.read_float(f) == status::error);
    }
    {
        // array of 2 with no element bytes.
        const unsigned char data[] = {0x82};
        reader<> in{data, sizeof(data)};
        std::vector<std::uint32_t> v;
        assert(in.read_value(v) == status::error);
    }
    {
        // empty window.
        const unsigned char data[] = {0x00};
        reader<> in{data, 0};
        std::uint64_t v = 0;
        assert(in.read_uint(v) == status::error);
    }
}

void test_invalid_header_errors() {
    {
        // reserved header byte.
        const unsigned char data[] = {0xff};
        reader<> in{data, sizeof(data)};
        std::uint64_t v = 0;
        assert(in.read_uint(v) == status::error);
    }
    {
        // small negative header is not a valid unsigned value.
        const unsigned char data[] = {0x40};
        reader<> in{data, sizeof(data)};
        std::uint64_t v = 0;
        assert(in.read_uint(v) == status::error);
    }
    {
        // signed int_payload header is not a valid unsigned value.
        const unsigned char data[] = {0x58, 0x11};
        reader<> in{data, sizeof(data)};
        std::uint64_t v = 0;
        assert(in.read_uint(v) == status::error);
    }
    {
        // reserved header on the skip path.
        const unsigned char data[] = {0xd0};
        reader<> in{data, sizeof(data)};
        assert(in.skip_value() == status::error);
    }
    {
        // reserved header for signed decode.
        const unsigned char data[] = {0xff};
        reader<> in{data, sizeof(data)};
        std::int64_t v = 0;
        assert(in.read_int(v) == status::error);
    }
}

void test_oversized_length_errors() {
    {
        // string extended size claims 1000000 bytes, buffer far too small.
        // 1000000 == 0x0F4240 -> unsigned int_payload of 3 bytes.
        const unsigned char data[] = {0x7f, 0x52, 0x40, 0x42, 0x0f, 'a', 'b'};
        reader<> in{data, sizeof(data)};
        std::string s;
        assert(in.read_value(s) == status::error);
    }
    {
        // array extended count claims 1000000 items, one element present.
        const unsigned char data[] = {0x8f, 0x52, 0x40, 0x42, 0x0f, 0x01};
        reader<> in{data, sizeof(data)};
        std::vector<std::uint32_t> v;
        assert(in.read_value(v) == status::error);
    }
    {
        // byte-like fixed std::array: wire claims 6 bytes, capacity is 4.
        const unsigned char data[] = {0x66, 'a', 'b', 'c', 'd', 'e', 'f'};
        reader<> in{data, sizeof(data)};
        std::array<unsigned char, 4> target{};
        assert(in.read_value(target) == status::error);
    }
    {
        // same, but for a raw C array target.
        const unsigned char data[] = {0x66, 'a', 'b', 'c', 'd', 'e', 'f'};
        reader<> in{data, sizeof(data)};
        unsigned char target[4]{};
        assert(in.read_value(target) == status::error);
    }
}

void test_integer_overflow_rejected() {
    {
        // wire value 300 does not fit std::uint8_t.
        // 300 == 0x012C -> unsigned int_payload of 2 bytes.
        const unsigned char data[] = {0x51, 0x2c, 0x01};
        reader<> in{data, sizeof(data)};
        std::uint8_t v = 0;
        assert(in.read_value(v) == status::error);
    }
    {
        // negative wire value decoded into an unsigned target.
        const unsigned char data[] = {0x40};
        reader<> in{data, sizeof(data)};
        std::uint32_t v = 0;
        assert(in.read_value(v) == status::error);
    }
}

} // namespace

int main() {
    test_integer_headers();
    test_size_headers();
    test_bool_null_and_float();
    test_skip_value();
    test_span_codec();
    test_bitset_codec();
    test_value_codecs();
    test_optional_codecs();
    test_vector_codecs();
    test_array_codecs();
    test_fixed_array_codecs();
    test_map_codecs();
    test_tuple_codecs();
    test_variant_codecs();
    test_contract_object_roundtrip();
    test_contract_object_vector_fields();
    test_contract_object_array_fields();
    test_contract_object_fixed_array_fields();
    test_contract_object_map_fields();
    test_contract_object_tuple_variant_fields();
    test_contract_object_missing_and_unknown_fields();
    test_contract_object_vector_max_items_error();
    test_contract_object_array_errors();
    test_contract_object_map_errors();
    test_contract_object_tuple_variant_errors();
    test_contract_object_out_of_order_fields();
    test_contract_object_duplicate_field_last_wins();
    test_contract_object_optional_fields();
    test_checked_io_errors();
    test_truncated_input_errors();
    test_invalid_header_errors();
    test_oversized_length_errors();
    test_integer_overflow_rejected();
}
