// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/binary/all.hpp>
#include <contract/contract.hpp>

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

struct BinaryInner {
    std::uint32_t kind = 7;
    std::string name = "inner";

    CONTRACT(BinaryInner, (kind, 1), (name, 2))
};

struct BinaryMixedRecord {
    std::string title = "integration";
    std::vector<std::uint32_t> ids{1, 2, 3};
    std::array<std::uint32_t, 3> codes{4, 5, 6};
    std::tuple<std::string, std::uint32_t, BinaryInner> payload{"slot", 17, BinaryInner{}};
    std::variant<std::uint32_t, std::string, BinaryInner> status{BinaryInner{}};
    std::optional<BinaryInner> maybe_inner{BinaryInner{}};
    std::map<std::uint32_t, std::string> labels{
        {1, "one"},
        {3, "three"},
    };
    std::unordered_map<std::uint32_t, std::string> lookup{
        {1, "one"},
        {3, "three"},
    };
    std::bitset<10> flags{0b1010010110};

    CONTRACT(BinaryMixedRecord,
        (title, 1),
        (ids, 2),
        (codes, 3),
        (payload, 4),
        (status, 5),
        (maybe_inner, 6),
        (labels, 7),
        (lookup, 8),
        (flags, 9))
};

void assert_equal(const BinaryMixedRecord& lhs, const BinaryMixedRecord& rhs) {
    assert(lhs.title == rhs.title);
    assert(lhs.ids == rhs.ids);
    assert(lhs.codes == rhs.codes);
    assert(std::get<0>(lhs.payload) == std::get<0>(rhs.payload));
    assert(std::get<1>(lhs.payload) == std::get<1>(rhs.payload));
    assert(std::get<2>(lhs.payload).kind == std::get<2>(rhs.payload).kind);
    assert(std::get<2>(lhs.payload).name == std::get<2>(rhs.payload).name);
    assert(lhs.status.index() == rhs.status.index());
    assert(lhs.maybe_inner.has_value() == rhs.maybe_inner.has_value());
    assert(lhs.labels == rhs.labels);
    assert(lhs.lookup == rhs.lookup);
    assert(lhs.flags == rhs.flags);

    assert(std::holds_alternative<BinaryInner>(lhs.status));
    assert(std::holds_alternative<BinaryInner>(rhs.status));
    const auto& lhs_status = std::get<BinaryInner>(lhs.status);
    const auto& rhs_status = std::get<BinaryInner>(rhs.status);
    assert(lhs_status.kind == rhs_status.kind);
    assert(lhs_status.name == rhs_status.name);

    assert(lhs.maybe_inner.has_value());
    assert(rhs.maybe_inner.has_value());
    assert(lhs.maybe_inner->kind == rhs.maybe_inner->kind);
    assert(lhs.maybe_inner->name == rhs.maybe_inner->name);
}

} // namespace

int main() {
    BinaryMixedRecord source;
    source.title = "integration";
    source.ids = {1, 2, 3, 4};
    source.codes = {4, 5, 6};
    source.payload = {"slot", 17, BinaryInner{11, "payload"}};
    source.status = BinaryInner{13, "status"};
    source.maybe_inner = BinaryInner{19, "maybe"};
    source.labels = {{1, "one"}, {3, "three"}};
    source.lookup = {{1, "one"}, {3, "three"}};
    source.flags = std::bitset<10>(0b1010010110);

    std::array<unsigned char, 512> buffer{};
    contract::adapters::binary::writer<> out(buffer.data());
    out << source;
    const auto size = static_cast<std::size_t>(out.current() - buffer.data());

    BinaryMixedRecord target{};
    target.title.clear();
    target.ids.clear();
    target.codes = {0, 0, 0};
    target.payload = {"", 0, BinaryInner{}};
    target.status = std::uint32_t{0};
    target.maybe_inner.reset();
    target.labels.clear();
    target.lookup.clear();
    target.flags.reset();

    contract::adapters::binary::reader<contract::io::checked_input> in(
        buffer.data(), size);
    in >> target;

    assert_equal(source, target);
    return 0;
}
