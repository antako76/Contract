// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary/all.hpp>
#include "contract_test_types.hpp"

#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <map>
#include <optional>
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>
#include <type_traits>
#include <utility>

namespace {

struct BinaryRecord {
    std::uint64_t id = 10;
    std::uint32_t count = 20;
    double ratio = 1.5;

    CONTRACT(BinaryRecord, (id, 1), (count, 2), (ratio, 3))
};

struct NestedChild {
    std::uint32_t value = 77;

    CONTRACT(NestedChild, (value, 1))
};

struct NestedParent {
    std::uint64_t id = 12;
    NestedChild child;

    CONTRACT(NestedParent, (id, 1), (child, 2))
};

struct HookedBinaryRecord {
    std::uint32_t count = 0;

    CONTRACT(HookedBinaryRecord, (count, 1))

    template<class Value>
    void contract_set(contract::tag<contract_field::count>, Value&& value) {
        count = static_cast<std::uint32_t>(std::forward<Value>(value)) + 1;
    }
};

struct TextBinaryRecord {
    std::string name = "contract";
    const char* category = nullptr;
    std::string category_storage = "adapter";

    TextBinaryRecord() {
        category = category_storage.c_str();
    }

    CONTRACT(TextBinaryRecord, (name, 1), (category, 2))

    template<class Value>
    void contract_set(contract::tag<contract_field::name>, Value&& value) {
        name = std::forward<Value>(value);
    }

    template<class Value>
    void contract_set(contract::tag<contract_field::category>, Value&& value) {
        category_storage = std::forward<Value>(value);
        category = category_storage.c_str();
    }
};

struct StringViewRecord {
    std::string_view name = "borrowed";

    CONTRACT(StringViewRecord, (name, 1))
};

struct HookedStringViewRecord {
    std::string_view name;

    CONTRACT(HookedStringViewRecord, (name, 1))

    template<class Value>
    void contract_set(contract::tag<contract_field::name>, Value&& value) {
        name = std::forward<Value>(value);
    }
};

struct FixedCharArrayRecord {
    std::uint64_t id = 31;
    char code[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};

    CONTRACT(FixedCharArrayRecord, (id, 1), (code, 2))
};

struct FixedUIntArrayRecord {
    std::uint32_t values[4] = {1, 2, 3, 4};

    CONTRACT(FixedUIntArrayRecord, (values, 1))
};

struct BitsetRecord {
    std::bitset<10> flags{0b1010010110};

    CONTRACT(BitsetRecord, (flags, 1))
};

struct ArrayRecord {
    std::array<std::uint32_t, 4> ids{1, 2, 3, 4};
    std::array<std::string, 2> names{"alpha", "beta"};

    CONTRACT(ArrayRecord, (ids, 1), (names, 2))
};

struct TupleRecord {
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        7,
        "tuple",
        {1, 2, 3},
    };

    CONTRACT(TupleRecord, (payload, 1))
};

struct VariantRecord {
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        std::string("variant")
    };

    CONTRACT(VariantRecord, (payload, 1))
};

struct VectorRecord {
    std::vector<std::uint32_t> ids{[] {
        std::vector<std::uint32_t> values;
        values.reserve(1000);
        for (std::uint32_t i = 0; i < 1000; ++i) {
            values.push_back(i + 1);
        }
        return values;
    }()};
    std::vector<std::string> names{"alpha", "beta"};

    CONTRACT(VectorRecord, (ids, 1), (names, 2))
};

struct MapRecord {
    std::map<std::uint32_t, std::string> labels{{1, "one"}, {3, "three"}};

    CONTRACT(MapRecord, (labels, 1))
};

struct OptionalRecord {
    std::optional<std::uint32_t> count{42};
    std::optional<std::string> name{"opt"};

    CONTRACT(OptionalRecord, (count, 1), (name, 2))
};

struct UnorderedMapRecord {
    std::unordered_map<std::uint32_t, std::string> labels{{3, "three"}, {1, "one"}};

    CONTRACT(UnorderedMapRecord, (labels, 1))
};

} // namespace

int main() {
    BinaryRecord source;
    std::array<unsigned char, sizeof(std::uint64_t) + sizeof(std::uint32_t) + sizeof(double)> buffer{};

    contract::adapters::binary::writer<> out(buffer.data());
    out << source;

    BinaryRecord target;
    target.id = 0;
    target.count = 0;
    target.ratio = 0.0;

    contract::adapters::binary::reader<> in(buffer.data());
    in >> target;

    assert(target.id == 10);
    assert(target.count == 20);
    assert(target.ratio == 1.5);

    using namespace contract_tests;

    RequestEvent request_source;
    request_source.timestamp = 101;
    request_source.service = 202;
    request_source.user_id = 303;

    std::array<unsigned char, sizeof(std::uint64_t) * 3> request_buffer{};
    contract::adapters::binary::writer<> request_out(request_buffer.data());
    request_out << request_source;

    RequestEvent request_target{};
    request_target.timestamp = 0;
    request_target.service = 0;
    request_target.user_id = 0;
    contract::adapters::binary::reader<> request_in(request_buffer.data());
    request_in >> request_target;
    assert(request_target.timestamp == 101);
    assert(request_target.service == 202);
    assert(request_target.user_id == 303);

    RoutedEvent routed_source;
    routed_source.timestamp = 11;
    routed_source.service = 22;
    routed_source.trace_id = 33;
    routed_source.route_id = 44;

    std::array<unsigned char, sizeof(std::uint64_t) * 4> routed_buffer{};
    contract::adapters::binary::writer<> routed_out(routed_buffer.data());
    routed_out << routed_source;

    RoutedEvent routed_target{};
    routed_target.timestamp = 0;
    routed_target.service = 0;
    routed_target.trace_id = 0;
    routed_target.route_id = 0;
    contract::adapters::binary::reader<> routed_in(routed_buffer.data());
    routed_in >> routed_target;
    assert(routed_target.timestamp == 11);
    assert(routed_target.service == 22);
    assert(routed_target.trace_id == 33);
    assert(routed_target.route_id == 44);

    NestedParent nested_source;
    std::array<unsigned char, sizeof(std::uint64_t) + sizeof(std::uint32_t)> nested_buffer{};
    contract::adapters::binary::writer<> nested_out(nested_buffer.data());
    nested_out << nested_source;

    NestedParent nested_target{};
    nested_target.id = 0;
    nested_target.child.value = 0;
    contract::adapters::binary::reader<> nested_in(nested_buffer.data());
    nested_in >> nested_target;
    assert(nested_target.id == 12);
    assert(nested_target.child.value == 77);

    bool truncated_read_failed = false;
    try {
        BinaryRecord truncated_target{};
        contract::adapters::binary::reader<contract::io::checked_input> checked_in(
            buffer.data(), sizeof(std::uint64_t));
        checked_in >> truncated_target;
    } catch (const std::runtime_error& error) {
        const std::string_view message = error.what();
        assert(message.find("truncated") != std::string_view::npos);
        assert(message.find("field count") != std::string_view::npos);
        truncated_read_failed = true;
    }
    assert(truncated_read_failed);

    using HookedField = std::remove_cv_t<std::remove_reference_t<decltype(contract::field_at<0, HookedBinaryRecord>())>>;
    static_assert(!contract::adapters::binary::can_direct_field_read_v<HookedField, HookedBinaryRecord>);

    std::array<unsigned char, sizeof(std::uint32_t)> hooked_buffer{};
    const std::uint32_t raw = 9;
    std::memcpy(hooked_buffer.data(), &raw, sizeof(raw));

    HookedBinaryRecord hooked{};
    contract::adapters::binary::reader<> in_hooked(hooked_buffer.data());
    in_hooked >> hooked;
    assert(hooked.count == 10);

    TextBinaryRecord text_source;
    std::array<unsigned char, 128> text_buffer{};
    contract::adapters::binary::writer<> out_text(text_buffer.data());
    out_text << text_source;

    StringViewRecord view_source;
    std::array<unsigned char, 64> view_buffer{};
    contract::adapters::binary::writer<> view_out(view_buffer.data());
    view_out << view_source;
    const std::size_t view_size = static_cast<std::size_t>(view_out.current() - view_buffer.data());

    StringViewRecord view_target{};
    view_target.name = {};
    contract::adapters::binary::reader<contract::io::checked_input> view_in(
        view_buffer.data(), view_size);
    view_in >> view_target;
    assert(view_target.name == "borrowed");
    assert(view_target.name.data() == reinterpret_cast<const char*>(view_buffer.data() + sizeof(std::size_t)));

    HookedStringViewRecord hooked_view_target{};
    contract::adapters::binary::reader<contract::io::checked_input> hooked_view_in(
        view_buffer.data(), view_size);
    hooked_view_in >> hooked_view_target;
    assert(hooked_view_target.name == "borrowed");
    assert(hooked_view_target.name.data() == reinterpret_cast<const char*>(view_buffer.data() + sizeof(std::size_t)));

    FixedCharArrayRecord fixed_chars_source;
    std::array<unsigned char, sizeof(std::uint64_t) + 8> fixed_chars_buffer{};
    contract::adapters::binary::writer<> fixed_chars_out(fixed_chars_buffer.data());
    fixed_chars_out << fixed_chars_source;

    FixedCharArrayRecord fixed_chars_target{};
    fixed_chars_target.id = 0;
    std::memset(fixed_chars_target.code, 0, sizeof(fixed_chars_target.code));
    contract::adapters::binary::reader<> fixed_chars_in(fixed_chars_buffer.data());
    fixed_chars_in >> fixed_chars_target;
    assert(fixed_chars_target.id == 31);
    assert(std::memcmp(fixed_chars_target.code, "contract", 8) == 0);

    FixedUIntArrayRecord fixed_uint_source;
    std::array<unsigned char, sizeof(fixed_uint_source.values)> fixed_uint_buffer{};
    contract::adapters::binary::writer<> fixed_uint_out(fixed_uint_buffer.data());
    fixed_uint_out << fixed_uint_source;

    FixedUIntArrayRecord fixed_uint_target{};
    fixed_uint_target.values[0] = 0;
    fixed_uint_target.values[1] = 0;
    fixed_uint_target.values[2] = 0;
    fixed_uint_target.values[3] = 0;
    contract::adapters::binary::reader<> fixed_uint_in(fixed_uint_buffer.data());
    fixed_uint_in >> fixed_uint_target;
    assert((fixed_uint_target.values[0] == fixed_uint_source.values[0]));
    assert((fixed_uint_target.values[1] == fixed_uint_source.values[1]));
    assert((fixed_uint_target.values[2] == fixed_uint_source.values[2]));
    assert((fixed_uint_target.values[3] == fixed_uint_source.values[3]));

    BitsetRecord bitset_source;
    std::array<unsigned char, sizeof(std::uint64_t)> bitset_buffer{};
    contract::adapters::binary::writer<> bitset_out(bitset_buffer.data());
    bitset_out << bitset_source;

    BitsetRecord bitset_target{};
    bitset_target.flags.reset();
    contract::adapters::binary::reader<> bitset_in(bitset_buffer.data());
    bitset_in >> bitset_target;
    assert(bitset_target.flags == bitset_source.flags);

    TupleRecord tuple_source;
    tuple_source.payload = {
        std::uint32_t{17},
        std::string("tuple-value"),
        std::array<std::uint32_t, 3>{4, 5, 6},
    };
    std::array<unsigned char, 128> tuple_buffer{};
    contract::adapters::binary::writer<> tuple_out(tuple_buffer.data());
    tuple_out << tuple_source;
    const std::size_t tuple_size = static_cast<std::size_t>(tuple_out.current() - tuple_buffer.data());

    TupleRecord tuple_target{};
    tuple_target.payload = {
        std::uint32_t{0},
        std::string(),
        std::array<std::uint32_t, 3>{},
    };
    contract::adapters::binary::reader<contract::io::checked_input> tuple_in(
        tuple_buffer.data(), tuple_size);
    tuple_in >> tuple_target;
    assert(std::get<0>(tuple_target.payload) == 17);
    assert(std::get<1>(tuple_target.payload) == "tuple-value");
    assert((std::get<2>(tuple_target.payload) == std::array<std::uint32_t, 3>{4, 5, 6}));

    VariantRecord variant_source;
    variant_source.payload = std::array<std::uint32_t, 3>{9, 8, 7};
    std::array<unsigned char, 128> variant_buffer{};
    contract::adapters::binary::writer<> variant_out(variant_buffer.data());
    variant_out << variant_source;
    const std::size_t variant_size = static_cast<std::size_t>(variant_out.current() - variant_buffer.data());

    VariantRecord variant_target{};
    variant_target.payload = std::uint32_t{0};
    contract::adapters::binary::reader<contract::io::checked_input> variant_in(
        variant_buffer.data(), variant_size);
    variant_in >> variant_target;
    assert((std::holds_alternative<std::array<std::uint32_t, 3>>(variant_target.payload)));
    assert((std::get<std::array<std::uint32_t, 3>>(variant_target.payload) == std::array<std::uint32_t, 3>{9, 8, 7}));

    ArrayRecord array_source;
    std::array<unsigned char, sizeof(std::uint32_t) * 4 + sizeof(std::size_t) * 2 + 16> array_buffer{};
    contract::adapters::binary::writer<> array_out(array_buffer.data());
    array_out << array_source;

    ArrayRecord array_target{};
    array_target.ids = {0, 0, 0, 0};
    array_target.names = {"", ""};
    contract::adapters::binary::reader<> array_in(array_buffer.data());
    array_in >> array_target;
    assert((array_target.ids == std::array<std::uint32_t, 4>{1, 2, 3, 4}));
    assert((array_target.names == std::array<std::string, 2>{"alpha", "beta"}));

    VectorRecord vector_source;
    std::vector<unsigned char> vector_buffer(16384);
    contract::adapters::binary::writer<> vector_out(vector_buffer.data());
    vector_out << vector_source;
    const std::size_t vector_size = static_cast<std::size_t>(vector_out.current() - vector_buffer.data());

    VectorRecord vector_target{};
    vector_target.ids.clear();
    vector_target.names.clear();
    contract::adapters::binary::reader<contract::io::checked_input> vector_in(
        vector_buffer.data(), vector_size);
    vector_in >> vector_target;
    assert(vector_target.ids.size() == 1000);
    assert(vector_target.ids.front() == 1);
    assert(vector_target.ids.back() == 1000);
    assert(vector_target.names == std::vector<std::string>({"alpha", "beta"}));

    MapRecord map_source;
    std::vector<unsigned char> map_buffer(256);
    contract::adapters::binary::writer<> map_out(map_buffer.data());
    map_out << map_source;
    const std::size_t map_size = static_cast<std::size_t>(map_out.current() - map_buffer.data());

    MapRecord map_target{};
    map_target.labels.clear();
    contract::adapters::binary::reader<contract::io::checked_input> map_in(
        map_buffer.data(), map_size);
    map_in >> map_target;
    assert((map_target.labels == std::map<std::uint32_t, std::string>{{1, "one"}, {3, "three"}}));

    bool duplicate_map_key_failed = false;
    try {
        std::array<unsigned char, sizeof(std::size_t) + 4 * sizeof(std::uint32_t)> duplicate_buffer{};
        std::size_t offset = 0;
        const std::size_t duplicate_count = 2;
        std::memcpy(duplicate_buffer.data() + offset, &duplicate_count, sizeof(duplicate_count));
        offset += sizeof(duplicate_count);

        const std::uint32_t key1 = 7;
        const std::uint32_t value1 = 1;
        const std::uint32_t key2 = 7;
        const std::uint32_t value2 = 2;
        std::memcpy(duplicate_buffer.data() + offset, &key1, sizeof(key1));
        offset += sizeof(key1);
        std::memcpy(duplicate_buffer.data() + offset, &value1, sizeof(value1));
        offset += sizeof(value1);
        std::memcpy(duplicate_buffer.data() + offset, &key2, sizeof(key2));
        offset += sizeof(key2);
        std::memcpy(duplicate_buffer.data() + offset, &value2, sizeof(value2));

        std::map<std::uint32_t, std::uint32_t> duplicate_target;
        contract::adapters::binary::reader<contract::io::checked_input> duplicate_in(
            duplicate_buffer.data(), duplicate_buffer.size());
        duplicate_in >> duplicate_target;
    } catch (const std::runtime_error& error) {
        const std::string_view message = error.what();
        assert(message.find("duplicate key") != std::string_view::npos);
        duplicate_map_key_failed = true;
    }
    assert(duplicate_map_key_failed);

    OptionalRecord optional_source;
    std::vector<unsigned char> optional_buffer(256);
    contract::adapters::binary::writer<> optional_out(optional_buffer.data());
    optional_out << optional_source;
    const std::size_t optional_size = static_cast<std::size_t>(optional_out.current() - optional_buffer.data());

    OptionalRecord optional_target{};
    optional_target.count.reset();
    optional_target.name.reset();
    contract::adapters::binary::reader<contract::io::checked_input> optional_in(
        optional_buffer.data(), optional_size);
    optional_in >> optional_target;
    assert(optional_target.count.has_value());
    assert(*optional_target.count == 42);
    assert(optional_target.name.has_value());
    assert(*optional_target.name == "opt");

    OptionalRecord empty_optional_source;
    empty_optional_source.count.reset();
    empty_optional_source.name.reset();
    std::vector<unsigned char> empty_optional_buffer(64);
    contract::adapters::binary::writer<> empty_optional_out(empty_optional_buffer.data());
    empty_optional_out << empty_optional_source;
    const std::size_t empty_optional_size = static_cast<std::size_t>(empty_optional_out.current() - empty_optional_buffer.data());

    OptionalRecord empty_optional_target{};
    empty_optional_target.count = 7;
    empty_optional_target.name = "keep";
    contract::adapters::binary::reader<contract::io::checked_input> empty_optional_in(
        empty_optional_buffer.data(), empty_optional_size);
    empty_optional_in >> empty_optional_target;
    assert(!empty_optional_target.count.has_value());
    assert(!empty_optional_target.name.has_value());

    UnorderedMapRecord unordered_source_a;
    UnorderedMapRecord unordered_source_b;
    unordered_source_b.labels.clear();
    unordered_source_b.labels.emplace(1, "one");
    unordered_source_b.labels.emplace(3, "three");

    std::vector<unsigned char> unordered_buffer_a(256);
    contract::adapters::binary::writer<> unordered_out_a(unordered_buffer_a.data());
    unordered_out_a << unordered_source_a;
    const std::size_t unordered_size_a = static_cast<std::size_t>(unordered_out_a.current() - unordered_buffer_a.data());

    std::vector<unsigned char> unordered_buffer_b(256);
    contract::adapters::binary::writer<> unordered_out_b(unordered_buffer_b.data());
    unordered_out_b << unordered_source_b;
    const std::size_t unordered_size_b = static_cast<std::size_t>(unordered_out_b.current() - unordered_buffer_b.data());

    UnorderedMapRecord unordered_target_a{};
    unordered_target_a.labels.clear();
    contract::adapters::binary::reader<contract::io::checked_input> unordered_in_a(
        unordered_buffer_a.data(), unordered_size_a);
    unordered_in_a >> unordered_target_a;
    assert((unordered_target_a.labels == std::unordered_map<std::uint32_t, std::string>{{1, "one"}, {3, "three"}}));

    UnorderedMapRecord unordered_target_b{};
    unordered_target_b.labels.clear();
    contract::adapters::binary::reader<contract::io::checked_input> unordered_in_b(
        unordered_buffer_b.data(), unordered_size_b);
    unordered_in_b >> unordered_target_b;
    assert((unordered_target_b.labels == std::unordered_map<std::uint32_t, std::string>{{1, "one"}, {3, "three"}}));

    bool duplicate_unordered_key_failed = false;
    try {
        std::array<unsigned char, sizeof(std::size_t) + 4 * sizeof(std::uint32_t)> unordered_duplicate_buffer{};
        std::size_t offset = 0;
        const std::size_t duplicate_count = 2;
        std::memcpy(unordered_duplicate_buffer.data() + offset, &duplicate_count, sizeof(duplicate_count));
        offset += sizeof(duplicate_count);

        const std::uint32_t key1 = 5;
        const std::uint32_t value1 = 11;
        const std::uint32_t key2 = 5;
        const std::uint32_t value2 = 22;
        std::memcpy(unordered_duplicate_buffer.data() + offset, &key1, sizeof(key1));
        offset += sizeof(key1);
        std::memcpy(unordered_duplicate_buffer.data() + offset, &value1, sizeof(value1));
        offset += sizeof(value1);
        std::memcpy(unordered_duplicate_buffer.data() + offset, &key2, sizeof(key2));
        offset += sizeof(key2);
        std::memcpy(unordered_duplicate_buffer.data() + offset, &value2, sizeof(value2));

        std::unordered_map<std::uint32_t, std::uint32_t> unordered_duplicate_target;
        contract::adapters::binary::reader<contract::io::checked_input> unordered_duplicate_in(
            unordered_duplicate_buffer.data(), unordered_duplicate_buffer.size());
        unordered_duplicate_in >> unordered_duplicate_target;
    } catch (const std::runtime_error& error) {
        const std::string_view message = error.what();
        assert(message.find("duplicate key") != std::string_view::npos);
        duplicate_unordered_key_failed = true;
    }
    assert(duplicate_unordered_key_failed);

    return 0;
}
