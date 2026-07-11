// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console.hpp>
#include <contract/contract.hpp>
#include <contract/security.hpp>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

struct SecretChild {
    std::string detail = "nested-secret";

    CONTRACT(SecretChild, (detail, 1))
};

struct SecretRecord {
    std::string password = "p@ssw0rd";
    std::vector<std::string> tokens{"a", "b"};
    SecretChild profile{};
    std::string note = "public";

    CONTRACT(SecretRecord,
        (password, 1, contract::security::secret()),
        (tokens, 2, contract::security::no_log()),
        (profile, 3, contract::security::sensitive()),
        (note, 4))
};

} // namespace

int main() {
    SecretRecord record;

    const std::string schema_expected =
        "SecretRecord:\n"
        "  password: <redacted> # #1 std::string, contract::security::secret()\n"
        "  tokens: <redacted> # #2 std::vector<std::string>, contract::security::no_log()\n"
        "  profile: <redacted> # #3 SecretChild, contract::security::sensitive()\n"
        "  note: \"public\" # #4 std::string\n";

    assert(contract::adapters::console::to_string(record) == schema_expected);

    contract::adapters::console::options value_only;
    value_only.output_mode = contract::adapters::console::options::mode::value;

    const std::string value_expected =
        "SecretRecord:\n"
        "  password: <redacted>\n"
        "  tokens: <redacted>\n"
        "  profile: <redacted>\n"
        "  note: \"public\"\n";

    assert(contract::adapters::console::to_string(record, value_only) == value_expected);

    return 0;
}
