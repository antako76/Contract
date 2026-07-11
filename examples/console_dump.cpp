// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/cout.hpp>
#include <contract/check.hpp>
#include <contract/adapters/json/all.hpp>

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <unordered_map>
#include <vector>

struct UserProfile {
    std::string name = "Vasay";
    std::string role = "admin";

    CONTRACT(UserProfile, 
        (name, 1, contract::check::max_length(200)), 
        (role, 2)
    )
};

struct RouteKey {
    std::string method = "POST";
    std::string path = "/api/payments/{id}";

    CONTRACT(RouteKey,
        (method, 1),
        (path, 2)
    )
};

bool operator<(const RouteKey& lhs, const RouteKey& rhs) {
    return std::tie(lhs.method, lhs.path) < std::tie(rhs.method, rhs.path);
}

struct RoutedEvent {
    std::string service = "payment-api";
    std::string operation = "payment.create";

    CONTRACT(RoutedEvent,
        (service, 1),
        (operation, 2)
    )
};

std::array<std::byte, 256> make_permissions_blob() {
    std::array<std::byte, 256> bytes{};
    bytes[0] = std::byte{0x10};
    bytes[1] = std::byte{0x2b};
    bytes[2] = std::byte{0x9a};

    for (std::size_t i = 3; i < bytes.size(); ++i) {
        bytes[i] = std::byte{static_cast<unsigned char>(i & 0xff)};
    }

    return bytes;
}

struct RequestEvent : public UserProfile {
    unsigned long long id = 42;
    std::string route = "/api/payments/{id}";
    std::vector<std::string> tags = {"payment", "critical"};
    std::tuple<std::string, std::uint32_t> stage{"payment", 2};
    std::optional<UserProfile> profile = UserProfile{"maria", "operator"};
    std::variant<std::string, std::uint32_t, UserProfile> status{"queued"};
    std::bitset<10> flags{0b1010010110};
    std::array<std::byte, 256> permissions = make_permissions_blob();
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

    CONTRACT(RequestEvent, 
        BASE(UserProfile, 20), 
        (id, 1, contract::check::max_value(2000)), 
        (route, 2), 
        (tags, 3), 
        (stage, 4), 
        (profile, 5), 
        (status, 6), 
        (flags, 7),
        (permissions, 8),
        (headers, 9), 
        (labels, 10, contract::security::no_log()),
        (routes, 11, contract::check::max_items(20))
    )
};

int main() {
    RequestEvent event;
    contract::adapters::console::options opt;
//    opt.max_byte_preview_length = 6;
    auto out = contract::cout.with(opt);

    std::cout << "# schema\n";
    out << event;

    std::cout << "\n# debug\n";
    out.debug() << event;

    std::cout << "\n# json\n";
    std::cout << contract::adapters::json::to_string(event) << '\n';

    return 0;
}
