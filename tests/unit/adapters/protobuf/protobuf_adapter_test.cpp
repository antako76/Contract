// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/protobuf/all.hpp>
#include <contract/contract.hpp>
#include <contract/io.hpp>

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <tuple>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

struct ProtoChild {
    std::uint32_t value = 7;
    std::string name = "kid";

    CONTRACT(ProtoChild, (value, 1), (name, 2))
};

struct ProtoParent {
    std::uint32_t id = 150;
    std::string label = "hi";
    ProtoChild child{};

    CONTRACT(ProtoParent, (id, 1), (label, 2), (child, 3))
};

struct ProtoRepeated {
    std::vector<std::uint32_t> values{};
    std::vector<ProtoChild> children{};

    CONTRACT(ProtoRepeated, (values, 4), (children, 5))
};

struct ProtoArray {
    std::array<std::uint32_t, 3> codes{};

    CONTRACT(ProtoArray, (codes, 7))
};

struct ProtoFixedCharArray {
    std::uint64_t id = 31;
    char code[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};

    CONTRACT(ProtoFixedCharArray, (id, 1), (code, 2))
};

struct ProtoFixedUIntArray {
    std::uint32_t values[4] = {1, 2, 3, 4};

    CONTRACT(ProtoFixedUIntArray, (values, 1))
};

struct ProtoByteStdArray {
    std::array<unsigned char, 4> data{0x11, 0x22, 0x33, 0x44};

    CONTRACT(ProtoByteStdArray, (data, 1))
};

struct ProtoPaddedCharArray {
    char code[16] = {'h', 'i'};

    CONTRACT(ProtoPaddedCharArray, (code, 1))
};

struct ProtoBitset {
    std::bitset<10> flags{};

    CONTRACT(ProtoBitset, (flags, 6))
};

struct ProtoMap {
    std::map<std::uint32_t, std::string> labels{};

    CONTRACT(ProtoMap, (labels, 8))
};

struct ProtoUnorderedMap {
    std::unordered_map<std::uint32_t, std::string> labels{};

    CONTRACT(ProtoUnorderedMap, (labels, 9))
};

struct ProtoVariant {
    std::variant<std::uint32_t, std::string, ProtoChild> payload{std::string("hi")};

    CONTRACT(ProtoVariant, (payload, 10))
};

struct ProtoTuple {
    std::tuple<std::uint32_t, std::string, ProtoChild> payload{17u, "slot", ProtoChild{7, "kid"}};

    CONTRACT(ProtoTuple, (payload, 11))
};

} // namespace

int main() {
    ProtoParent value{};
    std::array<unsigned char, 64> buffer{};

    contract::adapters::protobuf::writer<> out(contract::io::window_output{buffer.data(), buffer.size()});
    out << value;

    const unsigned char expected[] = {
        0x08, 0x96, 0x01,
        0x12, 0x02, 0x68, 0x69,
        0x1a, 0x07,
        0x08, 0x07,
        0x12, 0x03, 0x6b, 0x69, 0x64,
    };

    const std::size_t written = out.position();
    assert(written == sizeof(expected));
    for (std::size_t i = 0; i < sizeof(expected); ++i) {
        assert(buffer[i] == expected[i]);
    }

    ProtoParent parsed{};
    contract::adapters::protobuf::reader<> in(contract::io::window_input{buffer.data(), written});
    in >> parsed;

    assert(parsed.id == value.id);
    assert(parsed.label == value.label);
    assert(parsed.child.value == value.child.value);
    assert(parsed.child.name == value.child.name);

    const unsigned char shuffled[] = {
        0x1a, 0x07,
        0x08, 0x07,
        0x12, 0x03, 0x6b, 0x69, 0x64,
        0x12, 0x02, 0x68, 0x69,
        0x08, 0x96, 0x01,
    };

    ProtoParent shuffled_parsed{};
    contract::adapters::protobuf::reader<> shuffled_in(
        contract::io::window_input{shuffled, sizeof(shuffled)});
    shuffled_in >> shuffled_parsed;

    assert(shuffled_parsed.id == value.id);
    assert(shuffled_parsed.label == value.label);
    assert(shuffled_parsed.child.value == value.child.value);
    assert(shuffled_parsed.child.name == value.child.name);

    const unsigned char wire_mismatch[] = {
        0x08, 0x96, 0x01,
        0x10, 0x02,
        0x1a, 0x07,
        0x08, 0x07,
        0x12, 0x03, 0x6b, 0x69, 0x64,
    };

    ProtoParent wire_mismatch_parsed{};
    contract::adapters::protobuf::reader<> mismatch_in(
        contract::io::window_input{wire_mismatch, sizeof(wire_mismatch)});

    bool mismatch_thrown = false;
    try {
        mismatch_in >> wire_mismatch_parsed;
    } catch (const std::runtime_error& e) {
        mismatch_thrown = true;
        const std::string message = e.what();
        assert(message.find("wire type mismatch") != std::string::npos);
        assert(message.find("label") != std::string::npos);
        assert(message.find("ProtoParent") != std::string::npos);
    }
    assert(mismatch_thrown);

    const unsigned char truncated[] = {
        0x08, 0x96, 0x01,
        0x12, 0x02, 0x68,
    };

    ProtoParent truncated_parsed{};
    contract::adapters::protobuf::reader<> truncated_in(
        contract::io::window_input{truncated, sizeof(truncated)});

    bool truncated_thrown = false;
    try {
        truncated_in >> truncated_parsed;
    } catch (const std::runtime_error& e) {
        truncated_thrown = true;
        const std::string message = e.what();
        assert(message.find("truncated") != std::string::npos);
        assert(message.find("label") != std::string::npos);
        assert(message.find("ProtoParent") != std::string::npos);
    }
    assert(truncated_thrown);

    std::array<unsigned char, 1> tiny_buffer{};
    contract::adapters::protobuf::writer<> tiny_out(
        contract::io::window_output{tiny_buffer.data(), tiny_buffer.size()});

    bool writer_thrown = false;
    try {
        tiny_out << value;
    } catch (const std::runtime_error&) {
        writer_thrown = true;
        const auto& const_tiny_out = tiny_out;
        assert(const_tiny_out.error().has_value());
        const std::string message = tiny_out.error_message();
        assert(message.find("protobuf writer") != std::string::npos);
        assert(message.find("output error") != std::string::npos);
        assert(message.find("ProtoParent") != std::string::npos);
        assert(message.find("id") != std::string::npos);
    }
    assert(writer_thrown);

    ProtoRepeated repeated{};
    repeated.values = {1u, 2u, 300u};
    repeated.children.push_back(ProtoChild{7, "kid"});
    repeated.children.push_back(ProtoChild{8, "two"});

    std::array<unsigned char, 128> repeated_buffer{};
    contract::adapters::protobuf::writer<> repeated_out(
        contract::io::window_output{repeated_buffer.data(), repeated_buffer.size()});
    repeated_out << repeated;

    const unsigned char repeated_expected[] = {
        0x22, 0x04, 0x01, 0x02, 0xAC, 0x02,
        0x2a, 0x07, 0x08, 0x07, 0x12, 0x03, 0x6b, 0x69, 0x64,
        0x2a, 0x07, 0x08, 0x08, 0x12, 0x03, 0x74, 0x77, 0x6f,
    };

    const std::size_t repeated_written = repeated_out.position();
    assert(repeated_written == sizeof(repeated_expected));
    for (std::size_t i = 0; i < sizeof(repeated_expected); ++i) {
        assert(repeated_buffer[i] == repeated_expected[i]);
    }

    ProtoRepeated repeated_parsed{};
    contract::adapters::protobuf::reader<> repeated_in(
        contract::io::window_input{repeated_buffer.data(), repeated_written});
    repeated_in >> repeated_parsed;

    assert(repeated_parsed.values == repeated.values);
    assert(repeated_parsed.children.size() == repeated.children.size());
    assert(repeated_parsed.children[0].value == repeated.children[0].value);
    assert(repeated_parsed.children[0].name == repeated.children[0].name);
    assert(repeated_parsed.children[1].value == repeated.children[1].value);
    assert(repeated_parsed.children[1].name == repeated.children[1].name);

    const unsigned char malformed_children[] = {
        0x2a, 0x01, 0x00,
    };

    ProtoRepeated malformed_children_parsed{};
    contract::adapters::protobuf::reader<> malformed_children_in(
        contract::io::window_input{malformed_children, sizeof(malformed_children)});

    bool malformed_children_thrown = false;
    try {
        malformed_children_in >> malformed_children_parsed;
    } catch (const std::runtime_error& e) {
        malformed_children_thrown = true;
        const std::string message = e.what();
        assert(message.find("field number zero") != std::string::npos);
        assert(message.find("children") != std::string::npos);
        assert(message.find("element #0") != std::string::npos);
        // The trace points to where the error was first created (inside the
        // nested ProtoChild's own read_message), not to vector.hpp's transfer
        // point - error creation is attributed to its origin, not re-created
        // where a parent later transfers it. Deliberately no line number (goes
        // stale on unrelated protobuf.hpp edits) and no exact function
        // signature text (compiler-specific pretty-printing: clang emits
        // "T = (anonymous namespace)::ProtoChild", gcc emits
        // "with T = {anonymous}::ProtoChild" - neither is portable).
        assert(message.find("created at contract/adapters/protobuf.hpp") != std::string::npos);
        assert(message.find("read_message") != std::string::npos);
        assert(message.find("ProtoChild") != std::string::npos);
    }
    assert(malformed_children_thrown);

    std::array<unsigned char, 128> unpacked_buffer{};
    contract::adapters::protobuf::writer<> unpacked_out(
        contract::io::window_output{unpacked_buffer.data(), unpacked_buffer.size()},
        contract::adapters::protobuf::options{.pack_repeated_scalars = false});
    unpacked_out << repeated;

    const unsigned char unpacked_expected[] = {
        0x20, 0x01,
        0x20, 0x02,
        0x20, 0xAC, 0x02,
        0x2a, 0x07, 0x08, 0x07, 0x12, 0x03, 0x6b, 0x69, 0x64,
        0x2a, 0x07, 0x08, 0x08, 0x12, 0x03, 0x74, 0x77, 0x6f,
    };

    const std::size_t unpacked_written = unpacked_out.position();
    assert(unpacked_written == sizeof(unpacked_expected));
    for (std::size_t i = 0; i < sizeof(unpacked_expected); ++i) {
        assert(unpacked_buffer[i] == unpacked_expected[i]);
    }

    const unsigned char unpacked_values[] = {
        0x20, 0x01,
        0x20, 0x02,
        0x20, 0xAC, 0x02,
    };

    ProtoRepeated unpacked_values_parsed{};
    contract::adapters::protobuf::reader<> unpacked_values_in(
        contract::io::window_input{unpacked_values, sizeof(unpacked_values)});
    unpacked_values_in >> unpacked_values_parsed;

    assert(unpacked_values_parsed.values == repeated.values);
    assert(unpacked_values_parsed.children.empty());

    ProtoArray array_source{};
    array_source.codes = {1u, 2u, 300u};

    std::array<unsigned char, 16> array_buffer{};
    contract::adapters::protobuf::writer<> array_out(
        contract::io::window_output{array_buffer.data(), array_buffer.size()});
    array_out << array_source;

    const unsigned char array_expected[] = {
        0x3a, 0x04, 0x01, 0x02, 0xAC, 0x02,
    };

    const std::size_t array_written = array_out.position();
    assert(array_written == sizeof(array_expected));
    for (std::size_t i = 0; i < sizeof(array_expected); ++i) {
        assert(array_buffer[i] == array_expected[i]);
    }

    ProtoArray array_parsed{};
    contract::adapters::protobuf::reader<> array_in(
        contract::io::window_input{array_buffer.data(), array_written});
    array_in >> array_parsed;
    assert(array_parsed.codes == array_source.codes);

    const unsigned char malformed_array[] = {
        0x3a, 0x04, 0x01, 0x02, 0x03, 0x04,
    };

    ProtoArray malformed_array_parsed{};
    contract::adapters::protobuf::reader<> malformed_array_in(
        contract::io::window_input{malformed_array, sizeof(malformed_array)});

    bool malformed_array_thrown = false;
    try {
        malformed_array_in >> malformed_array_parsed;
    } catch (const std::runtime_error& e) {
        malformed_array_thrown = true;
        const std::string message = e.what();
        assert(message.find("invalid size") != std::string::npos);
        assert(message.find("codes") != std::string::npos);
        assert(message.find("ProtoArray") != std::string::npos);
    }
    assert(malformed_array_thrown);

    ProtoMap map_source{};
    map_source.labels.emplace(1u, "one");
    map_source.labels.emplace(3u, "three");

    std::array<unsigned char, 32> map_buffer{};
    contract::adapters::protobuf::writer<> map_out(
        contract::io::window_output{map_buffer.data(), map_buffer.size()});
    map_out << map_source;

    const unsigned char expected_map[] = {
        0x42, 0x07, 0x08, 0x01, 0x12, 0x03, 0x6f, 0x6e, 0x65,
        0x42, 0x09, 0x08, 0x03, 0x12, 0x05, 0x74, 0x68, 0x72, 0x65, 0x65,
    };

    const std::size_t map_written = map_out.position();
    assert(map_written == sizeof(expected_map));
    for (std::size_t i = 0; i < sizeof(expected_map); ++i) {
        assert(map_buffer[i] == expected_map[i]);
    }

    ProtoMap map_parsed{};
    contract::adapters::protobuf::reader<> map_in(
        contract::io::window_input{map_buffer.data(), map_written});
    map_in >> map_parsed;
    assert(map_parsed.labels == map_source.labels);

    const unsigned char duplicate_map[] = {
        0x42, 0x07, 0x08, 0x01, 0x12, 0x03, 0x6f, 0x6e, 0x65,
        0x42, 0x07, 0x08, 0x01, 0x12, 0x03, 0x74, 0x77, 0x6f,
    };

    ProtoMap duplicate_map_parsed{};
    contract::adapters::protobuf::reader<> duplicate_map_in(
        contract::io::window_input{duplicate_map, sizeof(duplicate_map)});
    duplicate_map_in >> duplicate_map_parsed;
    assert(duplicate_map_parsed.labels.at(1u) == "two");

    ProtoBitset bitset_source{};
    bitset_source.flags = std::bitset<10>(0b1010010110);

    std::array<unsigned char, 16> bitset_buffer{};
    contract::adapters::protobuf::writer<> bitset_out(
        contract::io::window_output{bitset_buffer.data(), bitset_buffer.size()});
    bitset_out << bitset_source;

    const unsigned char bitset_expected[] = {
        0x32, 0x02, 0x96, 0x02,
    };

    const std::size_t bitset_written = bitset_out.position();
    assert(bitset_written == sizeof(bitset_expected));
    for (std::size_t i = 0; i < sizeof(bitset_expected); ++i) {
        assert(bitset_buffer[i] == bitset_expected[i]);
    }

    ProtoBitset bitset_parsed{};
    contract::adapters::protobuf::reader<> bitset_in(
        contract::io::window_input{bitset_buffer.data(), bitset_written});
    bitset_in >> bitset_parsed;
    assert(bitset_parsed.flags == bitset_source.flags);

    const unsigned char malformed_bitset[] = {
        0x32, 0x01, 0x96,
    };

    ProtoBitset malformed_bitset_parsed{};
    contract::adapters::protobuf::reader<> malformed_bitset_in(
        contract::io::window_input{malformed_bitset, sizeof(malformed_bitset)});

    bool malformed_bitset_thrown = false;
    try {
        malformed_bitset_in >> malformed_bitset_parsed;
    } catch (const std::runtime_error& e) {
        malformed_bitset_thrown = true;
        const std::string message = e.what();
        assert(message.find("invalid size") != std::string::npos);
        assert(message.find("flags") != std::string::npos);
        assert(message.find("ProtoBitset") != std::string::npos);
    }
    assert(malformed_bitset_thrown);

    ProtoUnorderedMap unordered_source{};
    unordered_source.labels.emplace(1u, "one");
    unordered_source.labels.emplace(3u, "three");

    std::array<unsigned char, 32> unordered_buffer{};
    contract::adapters::protobuf::writer<> unordered_out(
        contract::io::window_output{unordered_buffer.data(), unordered_buffer.size()});
    unordered_out << unordered_source;

    ProtoUnorderedMap unordered_parsed{};
    contract::adapters::protobuf::reader<> unordered_in(
        contract::io::window_input{unordered_buffer.data(), unordered_out.position()});
    unordered_in >> unordered_parsed;
    assert(unordered_parsed.labels == unordered_source.labels);

    const unsigned char unordered_duplicate[] = {
        0x4a, 0x07, 0x08, 0x01, 0x12, 0x03, 0x6f, 0x6e, 0x65,
        0x4a, 0x07, 0x08, 0x01, 0x12, 0x03, 0x74, 0x77, 0x6f,
    };

    ProtoUnorderedMap unordered_duplicate_parsed{};
    contract::adapters::protobuf::reader<> unordered_duplicate_in(
        contract::io::window_input{unordered_duplicate, sizeof(unordered_duplicate)});
    unordered_duplicate_in >> unordered_duplicate_parsed;
    assert(unordered_duplicate_parsed.labels.at(1u) == "two");

    ProtoVariant variant_source{};

    std::array<unsigned char, 32> variant_buffer{};
    contract::adapters::protobuf::writer<> variant_out(
        contract::io::window_output{variant_buffer.data(), variant_buffer.size()});
    variant_out << variant_source;

    const unsigned char variant_expected[] = {
        0x52, 0x07, 0x08, 0x01, 0x12, 0x03, 0x02, 0x68, 0x69,
    };

    const std::size_t variant_written = variant_out.position();
    assert(variant_written == sizeof(variant_expected));
    for (std::size_t i = 0; i < sizeof(variant_expected); ++i) {
        assert(variant_buffer[i] == variant_expected[i]);
    }

    ProtoVariant variant_parsed{};
    contract::adapters::protobuf::reader<> variant_in(
        contract::io::window_input{variant_buffer.data(), variant_written});
    variant_in >> variant_parsed;
    assert((std::holds_alternative<std::string>(variant_parsed.payload)));
    assert((std::get<std::string>(variant_parsed.payload) == std::string("hi")));

    const unsigned char duplicate_variant[] = {
        0x52, 0x07, 0x08, 0x01, 0x12, 0x03, 0x02, 0x68, 0x69,
        0x52, 0x06, 0x08, 0x00, 0x12, 0x02, 0x96, 0x01,
    };

    ProtoVariant duplicate_variant_parsed{};
    contract::adapters::protobuf::reader<> duplicate_variant_in(
        contract::io::window_input{duplicate_variant, sizeof(duplicate_variant)});
    duplicate_variant_in >> duplicate_variant_parsed;
    assert((std::holds_alternative<std::uint32_t>(duplicate_variant_parsed.payload)));
    assert((std::get<std::uint32_t>(duplicate_variant_parsed.payload) == 150u));

    ProtoVariant child_variant_source{};
    child_variant_source.payload = ProtoChild{7, "kid"};

    std::array<unsigned char, 32> child_variant_buffer{};
    contract::adapters::protobuf::writer<> child_variant_out(
        contract::io::window_output{child_variant_buffer.data(), child_variant_buffer.size()});
    child_variant_out << child_variant_source;

    const unsigned char child_variant_expected[] = {
        0x52, 0x0b, 0x08, 0x02, 0x12, 0x07, 0x08, 0x07, 0x12, 0x03, 0x6b, 0x69, 0x64,
    };

    const std::size_t child_variant_written = child_variant_out.position();
    assert(child_variant_written == sizeof(child_variant_expected));
    for (std::size_t i = 0; i < sizeof(child_variant_expected); ++i) {
        assert(child_variant_buffer[i] == child_variant_expected[i]);
    }

    ProtoVariant child_variant_parsed{};
    contract::adapters::protobuf::reader<> child_variant_in(
        contract::io::window_input{child_variant_buffer.data(), child_variant_written});
    child_variant_in >> child_variant_parsed;
    assert((std::holds_alternative<ProtoChild>(child_variant_parsed.payload)));
    assert((std::get<ProtoChild>(child_variant_parsed.payload).value == 7u));
    assert((std::get<ProtoChild>(child_variant_parsed.payload).name == std::string("kid")));

    ProtoTuple tuple_source{};

    std::array<unsigned char, 64> tuple_buffer{};
    contract::adapters::protobuf::writer<> tuple_out(
        contract::io::window_output{tuple_buffer.data(), tuple_buffer.size()});
    tuple_out << tuple_source;

    const unsigned char tuple_expected[] = {
        0x5a, 0x11,
        0x08, 0x11,
        0x12, 0x04, 0x73, 0x6c, 0x6f, 0x74,
        0x1a, 0x07, 0x08, 0x07, 0x12, 0x03, 0x6b, 0x69, 0x64,
    };

    const std::size_t tuple_written = tuple_out.position();
    assert(tuple_written == sizeof(tuple_expected));
    for (std::size_t i = 0; i < sizeof(tuple_expected); ++i) {
        assert(tuple_buffer[i] == tuple_expected[i]);
    }

    ProtoTuple tuple_parsed{};
    contract::adapters::protobuf::reader<> tuple_in(
        contract::io::window_input{tuple_buffer.data(), tuple_written});
    tuple_in >> tuple_parsed;
    assert((std::get<0>(tuple_parsed.payload) == 17u));
    assert((std::get<1>(tuple_parsed.payload) == std::string("slot")));
    assert((std::get<2>(tuple_parsed.payload).value == 7u));
    assert((std::get<2>(tuple_parsed.payload).name == std::string("kid")));

    const unsigned char duplicate_tuple[] = {
        0x5a, 0x13,
        0x08, 0x11,
        0x08, 0x12,
        0x12, 0x04, 0x73, 0x6c, 0x6f, 0x74,
        0x1a, 0x07, 0x08, 0x07, 0x12, 0x03, 0x6b, 0x69, 0x64,
    };

    ProtoTuple duplicate_tuple_parsed{};
    contract::adapters::protobuf::reader<> duplicate_tuple_in(
        contract::io::window_input{duplicate_tuple, sizeof(duplicate_tuple)});

    bool duplicate_tuple_thrown = false;
    try {
        duplicate_tuple_in >> duplicate_tuple_parsed;
    } catch (const std::runtime_error& e) {
        duplicate_tuple_thrown = true;
        const std::string message = e.what();
        assert(message.find("duplicate field") != std::string::npos);
    }
    assert(duplicate_tuple_thrown);

    const unsigned char missing_tuple[] = {
        0x5a, 0x08,
        0x08, 0x11,
        0x12, 0x04, 0x73, 0x6c, 0x6f, 0x74,
    };

    ProtoTuple missing_tuple_parsed{};
    contract::adapters::protobuf::reader<> missing_tuple_in(
        contract::io::window_input{missing_tuple, sizeof(missing_tuple)});

    bool missing_tuple_thrown = false;
    try {
        missing_tuple_in >> missing_tuple_parsed;
    } catch (const std::runtime_error& e) {
        missing_tuple_thrown = true;
        const std::string message = e.what();
        assert(message.find("invalid size") != std::string::npos);
    }
    assert(missing_tuple_thrown);

    // Robustness of the varint/tag/framing layer against untrusted bytes.
    // Each case must throw rather than over-read the window or loop forever.
    const auto expect_throw = [](const unsigned char* data, std::size_t size,
                                 const char* needle) {
        ProtoParent parsed_local{};
        contract::adapters::protobuf::reader<> reader_local(
            contract::io::window_input{data, size});
        bool thrown = false;
        try {
            reader_local >> parsed_local;
        } catch (const std::runtime_error& e) {
            thrown = true;
            assert(std::string(e.what()).find(needle) != std::string::npos);
        }
        assert(thrown);
    };

    // Varint value with the continuation bit set but no following byte.
    const unsigned char truncated_varint[] = {0x08, 0x96};
    expect_throw(truncated_varint, sizeof(truncated_varint), "truncated");

    // Varint that never terminates within the 10-byte maximum.
    const unsigned char overlong_varint[] = {
        0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    expect_throw(overlong_varint, sizeof(overlong_varint), "invalid varint");

    // Tag with field number zero at the top level.
    const unsigned char field_zero[] = {0x00, 0x01};
    expect_throw(field_zero, sizeof(field_zero), "field number zero");

    // Field number the message does not declare (protobuf rejects unknown fields).
    const unsigned char unknown_field[] = {0x78, 0x01};
    expect_throw(unknown_field, sizeof(unknown_field), "unknown field");

    // Tag varint truncated mid-encoding.
    const unsigned char truncated_tag[] = {0x80};
    expect_throw(truncated_tag, sizeof(truncated_tag), "truncated");

    // Length-delimited length that overruns the remaining buffer.
    const unsigned char length_overrun[] = {0x12, 0x7f, 0x68, 0x69};
    expect_throw(length_overrun, sizeof(length_overrun), "truncated");

    {
        // char[N]: byte-like -> raw bytes wire shape (like std::string), not
        // packed-repeated varints. Fully packed, nothing to trim.
        ProtoFixedCharArray source{};

        std::array<unsigned char, 32> buffer{};
        contract::adapters::protobuf::writer<> out(
            contract::io::window_output{buffer.data(), buffer.size()});
        out << source;

        const unsigned char expected[] = {
            0x08, 0x1f,
            0x12, 0x08, 'c', 'o', 'n', 't', 'r', 'a', 'c', 't',
        };
        assert(out.position() == sizeof(expected));
        for (std::size_t i = 0; i < sizeof(expected); ++i) {
            assert(buffer[i] == expected[i]);
        }

        ProtoFixedCharArray target{};
        std::memset(target.code, 0, sizeof(target.code));
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{buffer.data(), out.position()});
        in >> target;
        assert(target.id == source.id);
        assert(std::memcmp(target.code, source.code, sizeof(source.code)) == 0);
    }

    {
        // uint32_t[N]: not byte-like -> packed-repeated varints, unaffected.
        ProtoFixedUIntArray source{};

        std::array<unsigned char, 32> buffer{};
        contract::adapters::protobuf::writer<> out(
            contract::io::window_output{buffer.data(), buffer.size()});
        out << source;

        const unsigned char expected[] = {
            0x0a, 0x04, 0x01, 0x02, 0x03, 0x04,
        };
        assert(out.position() == sizeof(expected));
        for (std::size_t i = 0; i < sizeof(expected); ++i) {
            assert(buffer[i] == expected[i]);
        }

        ProtoFixedUIntArray target{};
        std::memset(target.values, 0, sizeof(target.values));
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{buffer.data(), out.position()});
        in >> target;
        assert(std::memcmp(target.values, source.values, sizeof(source.values)) == 0);
    }

    {
        // std::array<unsigned char, N> also takes the byte-like raw path.
        ProtoByteStdArray source{};

        std::array<unsigned char, 32> buffer{};
        contract::adapters::protobuf::writer<> out(
            contract::io::window_output{buffer.data(), buffer.size()});
        out << source;

        const unsigned char expected[] = {
            0x0a, 0x04, 0x11, 0x22, 0x33, 0x44,
        };
        assert(out.position() == sizeof(expected));
        for (std::size_t i = 0; i < sizeof(expected); ++i) {
            assert(buffer[i] == expected[i]);
        }

        ProtoByteStdArray target{};
        target.data = {};
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{buffer.data(), out.position()});
        in >> target;
        assert(target.data == source.data);
    }

    {
        // char[N] with trailing zero padding: trimmed on write, and the
        // reader must zero-fill the tail even over stale target contents.
        ProtoPaddedCharArray source{};

        std::array<unsigned char, 32> buffer{};
        contract::adapters::protobuf::writer<> out(
            contract::io::window_output{buffer.data(), buffer.size()});
        out << source;

        const unsigned char expected[] = {
            0x0a, 0x02, 'h', 'i',
        };
        assert(out.position() == sizeof(expected));
        for (std::size_t i = 0; i < sizeof(expected); ++i) {
            assert(buffer[i] == expected[i]);
        }

        ProtoPaddedCharArray target{};
        std::memset(target.code, 0xee, sizeof(target.code));
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{buffer.data(), out.position()});
        in >> target;
        assert(std::memcmp(target.code, source.code, sizeof(source.code)) == 0);
    }

    {
        // Byte-like fixed array: wire claims 6 bytes, capacity is 4 ->
        // must reject rather than overrun the target buffer.
        const unsigned char oversized_bytes[] = {
            0x0a, 0x06, 'a', 'b', 'c', 'd', 'e', 'f',
        };
        ProtoByteStdArray parsed{};
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{oversized_bytes, sizeof(oversized_bytes)});
        bool thrown = false;
        try {
            in >> parsed;
        } catch (const std::runtime_error& e) {
            thrown = true;
            const std::string message = e.what();
            assert(message.find("invalid size") != std::string::npos);
        }
        assert(thrown);
    }

    {
        // read_varint's SWAR fast path decodes up to 8 bytes at once; make
        // sure every byte-length class (1..10 bytes) round-trips correctly,
        // including the terminal_index==7 edge (exactly 8 bytes) and the
        // 9-10 byte tail path (negative-number sign extension).
        const std::uint64_t boundary_values[] = {
            0ull, 1ull, 127ull,
            128ull, 16383ull,
            16384ull, 2097151ull,
            2097152ull, 268435455ull,
            268435456ull, 34359738367ull,
            34359738368ull, 4398046511103ull,
            4398046511104ull, 562949953421311ull,
            562949953421312ull, 72057594037927935ull,
            72057594037927936ull, 9223372036854775807ull,
            static_cast<std::uint64_t>(-1),
            static_cast<std::uint64_t>(-12345),
        };

        for (const std::uint64_t value : boundary_values) {
            std::array<unsigned char, 16> buffer{};
            contract::adapters::protobuf::writer<> out(
                contract::io::window_output{buffer.data(), buffer.size()});
            const auto encode_status = out.write_varint(value);
            assert(encode_status == contract::adapters::base::status::ok);
            const std::size_t written = out.position();

            // Buffer exactly as long as the encoding: exercises the fast
            // path at its own boundary (window.size() >= 8 only when the
            // encoding itself is >= 8 bytes) and the tail path otherwise.
            std::uint64_t decoded = 0;
            contract::adapters::protobuf::reader<> exact_in(
                contract::io::window_input{buffer.data(), written});
            assert(exact_in.read_varint(decoded) == contract::adapters::base::status::ok);
            assert(decoded == value);
            assert(exact_in.position() == written);

            // Buffer padded past the encoding: always exercises the fast
            // path when the encoding fits within the first 8 padded bytes.
            std::uint64_t decoded_padded = 0;
            contract::adapters::protobuf::reader<> padded_in(
                contract::io::window_input{buffer.data(), buffer.size()});
            assert(padded_in.read_varint(decoded_padded) == contract::adapters::base::status::ok);
            assert(decoded_padded == value);
            assert(padded_in.position() == written);
        }
    }

    {
        // Truncated varint: buffer ends one byte short of what the
        // encoding needs, for both a small (tail path) and a >=8-byte
        // (fast path falls through) value.
        const unsigned char truncated_small[] = {0x80};
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{truncated_small, sizeof(truncated_small)});
        std::uint64_t decoded = 0;
        assert(in.read_varint(decoded) == contract::adapters::base::status::error);
        assert(in.error_message().find("truncated") != std::string::npos);
    }

    {
        const unsigned char truncated_large[] = {
            0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        };
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{truncated_large, sizeof(truncated_large)});
        std::uint64_t decoded = 0;
        assert(in.read_varint(decoded) == contract::adapters::base::status::error);
        assert(in.error_message().find("truncated") != std::string::npos);
    }

    {
        // Malformed varint: 10 bytes, every one with the continuation bit
        // set - never terminates within the wire format's own length cap.
        const unsigned char malformed[] = {
            0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        };
        contract::adapters::protobuf::reader<> in(
            contract::io::window_input{malformed, sizeof(malformed)});
        std::uint64_t decoded = 0;
        assert(in.read_varint(decoded) == contract::adapters::base::status::error);
        assert(in.error_message().find("invalid varint") != std::string::npos);
    }

    return 0;
}
