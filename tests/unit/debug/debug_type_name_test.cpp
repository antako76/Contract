// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include "contract_test_types.hpp"

#include <contract/adapters/debug/type_name.hpp>

#include <array>
#include <cassert>
#include <bitset>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

int main() {
    using namespace contract_tests;

    assert(contract::adapters::debug::type_name<BaseCounters>() == "BaseCounters");
    assert(contract::adapters::debug::type_name<const RequestEvent&>() == "RequestEvent");
    assert(contract::adapters::debug::type_name<volatile unsigned long long>() == "volatile u64");
    assert(contract::adapters::debug::type_name<const std::string&>() == "const std::string");

    assert(contract::adapters::debug::type_name<bool>() == "bool");
    assert(contract::adapters::debug::type_name<std::uint32_t>() == "u32");
    assert(contract::adapters::debug::type_name<std::int64_t>() == "i64");
    assert(contract::adapters::debug::type_name<double>() == "double");
    assert(contract::adapters::debug::type_name<std::string>() == "std::string");
    assert(contract::adapters::debug::type_name<std::string_view>() == "std::string_view");

    assert(contract::adapters::debug::type_name<std::vector<std::uint32_t>>() == "std::vector<u32>");
    assert((contract::adapters::debug::type_name<std::array<std::uint32_t, 4>>() == "std::array<u32,4>"));
    assert(contract::adapters::debug::type_name<std::optional<std::string>>() == "std::optional<std::string>");
    assert((contract::adapters::debug::type_name<std::map<std::string, std::uint32_t>>() == "std::map<std::string,u32>"));
    assert((contract::adapters::debug::type_name<std::unordered_map<std::string, std::uint32_t>>() == "std::unordered_map<std::string,u32>"));
    assert((contract::adapters::debug::type_name<std::tuple<std::uint32_t, std::string>>() == "std::tuple<u32,std::string>"));
    assert((contract::adapters::debug::type_name<std::variant<std::uint32_t, std::string>>() == "std::variant<u32,std::string>"));
    assert((contract::adapters::debug::type_name<std::bitset<10>>() == "std::bitset<10>"));
    assert((contract::adapters::debug::type_name<std::uint32_t[4]>() == "u32[4]"));

    return 0;
}
