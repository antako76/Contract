// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/binary/optional.hpp>
#include <contract/adapters/binary/vector.hpp>
#include <contract/cout.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

struct UserProfile {
    std::string username;
    std::string team;
    unsigned int employee_id;

    CONTRACT(UserProfile,
        (username, 1),
        (team, 2),
        (employee_id, 3)
    )
};

struct SourceInfo {
    std::string source;
    std::string tenant;

    CONTRACT(SourceInfo,
        (source, 1),
        (tenant, 2)
    )
};

struct SearchDocument : public SourceInfo {
    unsigned long long document_id;
    std::string title;
    std::vector<std::string> tags;
    std::optional<UserProfile> owner;

    CONTRACT(SearchDocument,
        BASE(SourceInfo, 10),
        (document_id, 1),
        (title, 2),
        (tags, 3),
        (owner, 4)
    )
};

int main() {
    SearchDocument event{
        SourceInfo{"ingest/api", "payments"},
        42,
        "Payment retry policy",
        {"payments", "retry", "critical"},
        UserProfile{"alex", "search", 1042}
    };
    std::array<unsigned char, 1024> buffer{};
    SearchDocument restored{};

    contract::cout.debug() << event;

    contract::adapters::binary::writer<> out(buffer.data());
    out << event;

    contract::adapters::binary::reader<> in(buffer.data());
    in >> restored;

    contract::cout.debug() << restored;

    return 0;
}
