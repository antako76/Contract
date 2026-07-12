// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/yaml/all.hpp>
#include <contract/contract.hpp>
#include <contract/io.hpp>

#include <cassert>
#include <optional>
#include <string>

namespace {

struct Person {
    std::string name;
    int age = 0;
    std::optional<std::string> nickname;

    CONTRACT(Person, (name, 1), (age, 2), (nickname, 3))
};

using contract::adapters::yaml::options;
using contract::adapters::yaml::parse_status;
using contract::adapters::yaml::reader;

} // namespace

int main() {
    {
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\n"});
        Person p{};
        assert(in.read(p) == parse_status::ok);
        assert(p.name == "Alice" && p.age == 30);
        assert(!p.nickname.has_value());
    }

    // Duplicate key: rejected by default.
    {
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\nage: 40\n"});
        Person p{};
        assert(in.read(p) == parse_status::error);
    }

    // Duplicate key: allowed when opted out, last value wins.
    {
        options opt{};
        opt.fail_on_duplicate_keys = false;
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\nage: 40\n"}, opt);
        Person p{};
        assert(in.read(p) == parse_status::ok);
        assert(p.age == 40);
    }

    // Unknown key: rejected by default.
    {
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\nbogus: xyz\n"});
        Person p{};
        assert(in.read(p) == parse_status::error);
    }

    // Unknown key: ignored when opted out.
    {
        options opt{};
        opt.fail_on_unknown_keys = false;
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\nbogus: xyz\n"}, opt);
        Person p{};
        assert(in.read(p) == parse_status::ok);
        assert(p.name == "Alice" && p.age == 30);
    }

    // Missing required (non-optional) key is an error.
    {
        reader<> in(contract::io::window_input{"age: 30\n"});
        Person p{};
        assert(in.read(p) == parse_status::error);
    }

    // Missing optional key just leaves it unset.
    {
        reader<> in(contract::io::window_input{"name: Alice\nage: 30\n"});
        Person p{};
        assert(in.read(p) == parse_status::ok);
        assert(!p.nickname.has_value());
    }

    return 0;
}
