// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/check.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>

namespace {

struct RouteKey {
    std::string method = "POST";
    std::string path = "/api/payments/{id}";

    CONTRACT(RouteKey, (method, 1), (path, 2))
};

bool operator<(const RouteKey& lhs, const RouteKey& rhs) {
    return std::tie(lhs.method, lhs.path) < std::tie(rhs.method, rhs.path);
}

struct RoutedEvent {
    std::string service = "payment-api";
    std::string operation = "payment.create";

    CONTRACT(RoutedEvent, (service, 1), (operation, 2))
};

struct MapRecord {
    std::map<std::string, std::string> headers{
        {"content-type", "application/json"},
        {"x-request-id", "abc"},
    };
    std::unordered_map<std::string, std::string> labels{
        {"priority", "high"},
    };
    std::map<RouteKey, RoutedEvent> routes{
        {RouteKey{}, RoutedEvent{}},
    };

    CONTRACT(MapRecord, (headers, 1), (labels, 2), (routes, 3, contract::check::max_items(20)))
};

} // namespace

int main() {
    MapRecord record;

    const std::string schema_expected =
        "MapRecord:\n"
        "  headers: # #1 std::map<std::string,std::string>, size=2\n"
        "    \"content-type\": \"application/json\" # [0]\n"
        "    \"x-request-id\": \"abc\" # [1]\n"
        "  labels: # #2 std::unordered_map<std::string,std::string>, size=1\n"
        "    \"priority\": \"high\" # [0]\n"
        "  routes: # #3 std::map<RouteKey,RoutedEvent>, size=1, contract::check::max_items(20)\n"
        "    - RouteKey: # [0] key\n"
        "        method: \"POST\" # #1 std::string\n"
        "        path: \"/api/payments/{id}\" # #2 std::string\n"
        "      RoutedEvent: # [0] value\n"
        "        service: \"payment-api\" # #1 std::string\n"
        "        operation: \"payment.create\" # #2 std::string\n";

    assert(contract::adapters::console::to_string(record) == schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;

    const std::string value_expected =
        "MapRecord:\n"
        "  headers:\n"
        "    \"content-type\": \"application/json\"\n"
        "    \"x-request-id\": \"abc\"\n"
        "  labels:\n"
        "    \"priority\": \"high\"\n"
        "  routes:\n"
        "    - RouteKey:\n"
        "        method: \"POST\"\n"
        "        path: \"/api/payments/{id}\"\n"
        "      RoutedEvent:\n"
        "        service: \"payment-api\"\n"
        "        operation: \"payment.create\"\n";

    assert(contract::adapters::console::to_string(record, value_only) == value_expected);

    contract::adapters::console::options schema_no_indexes;
    schema_no_indexes.output_mode = contract::adapters::console::options::mode::schema;
    schema_no_indexes.show_indexes = false;

    const std::string no_indexes_expected =
        "MapRecord:\n"
        "  headers: # #1 std::map<std::string,std::string>, size=2\n"
        "    \"content-type\": \"application/json\"\n"
        "    \"x-request-id\": \"abc\"\n"
        "  labels: # #2 std::unordered_map<std::string,std::string>, size=1\n"
        "    \"priority\": \"high\"\n"
        "  routes: # #3 std::map<RouteKey,RoutedEvent>, size=1, contract::check::max_items(20)\n"
        "    - RouteKey:\n"
        "        method: \"POST\" # #1 std::string\n"
        "        path: \"/api/payments/{id}\" # #2 std::string\n"
        "      RoutedEvent:\n"
        "        service: \"payment-api\" # #1 std::string\n"
        "        operation: \"payment.create\" # #2 std::string\n";

    assert(contract::adapters::console::to_string(record, schema_no_indexes) == no_indexes_expected);

    return 0;
}
