// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/yaml/all.hpp>
#include <contract/contract.hpp>
#include <contract/io.hpp>

#include <cassert>
#include <cstdint>
#include <fstream>
#include <limits>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

struct Limits {
    std::uint32_t max_items = 0;
    double ratio = 0.0;

    CONTRACT(Limits, (max_items, 1), (ratio, 2))
};

struct Config {
    std::string service;
    bool enabled = false;
    Limits limits{};
    std::vector<std::string> tags;
    std::vector<std::uint32_t> ports;
    std::tuple<std::uint32_t, std::string> stage{};
    std::optional<std::string> note{};
    std::string_view owner;

    CONTRACT(Config,
        (service, 1),
        (enabled, 2),
        (limits, 3),
        (tags, 4),
        (ports, 5),
        (stage, 6),
        (note, 7),
        (owner, 8))
};

struct LimitList {
    std::vector<Limits> limits;

    CONTRACT(LimitList, (limits, 1))
};

enum class Mode : std::uint32_t {
    alpha = 1,
    beta = 2,
};

struct NumericSample {
    std::int64_t signed_value = 0;
    std::uint64_t unsigned_value = 0;
    double ratio = 0.0;

    CONTRACT(NumericSample, (signed_value, 1), (unsigned_value, 2), (ratio, 3))
};

struct EnumSample {
    Mode mode = Mode::alpha;

    CONTRACT(EnumSample, (mode, 1))
};

struct NestedOptional {
    std::optional<std::vector<std::uint32_t>> values{};

    CONTRACT(NestedOptional, (values, 1))
};

struct DeepLevel4 {
    std::uint32_t value = 0;

    CONTRACT(DeepLevel4, (value, 1))
};

struct DeepLevel3 {
    DeepLevel4 level4{};

    CONTRACT(DeepLevel3, (level4, 1))
};

struct DeepLevel2 {
    DeepLevel3 level3{};

    CONTRACT(DeepLevel2, (level3, 1))
};

struct DeepLevel1 {
    DeepLevel2 level2{};

    CONTRACT(DeepLevel1, (level2, 1))
};

struct DeepTuple {
    std::tuple<std::uint32_t, std::string, std::uint32_t> stage{};

    CONTRACT(DeepTuple, (stage, 1))
};

struct ChunkWindowInput {
    std::string_view input;
    std::size_t pos = 0;
    std::size_t chunk = 3;

    std::span<const std::byte> peek(std::size_t size) const {
        if (pos >= input.size()) {
            return {};
        }

        const std::size_t available = input.size() - pos;
        std::size_t count = size < available ? size : available;
        count = count < chunk ? count : chunk;
        return {reinterpret_cast<const std::byte*>(input.data() + pos), count};
    }

    void consume(std::size_t size) {
        pos += size;
    }
};

void expect_parse_error(std::string_view yaml, std::string_view needle) {
    bool failed = false;
    try {
        Config cfg{};
        contract::adapters::yaml::reader<> in(contract::io::window_input{yaml});
        in >> cfg;
    } catch (const contract::adapters::yaml::parse_error& e) {
        failed = std::string_view(e.what()).find(needle) != std::string_view::npos;
    }
    assert(failed);
}

void expect_status_error(std::string_view yaml, std::string_view needle) {
    Config cfg{};
    contract::adapters::yaml::reader<> in(contract::io::window_input{yaml});
    const auto status = in.read(cfg);

    assert(status == contract::adapters::yaml::parse_status::error);
    const auto& error = static_cast<const contract::adapters::yaml::reader<>&>(in).error();
    assert(error.has_value());

    const std::string message = error->message();
    assert(message.find(needle) != std::string::npos);
    assert(message.find("created at contract/adapters/yaml.hpp:") != std::string::npos);
}

template<class T>
void expect_parse_error_for(std::string_view yaml, std::string_view needle) {
    bool failed = false;
    try {
        T value{};
        contract::adapters::yaml::reader<> in(contract::io::window_input{yaml});
        in >> value;
    } catch (const contract::adapters::yaml::parse_error& e) {
        failed = std::string_view(e.what()).find(needle) != std::string_view::npos;
    } catch (const std::runtime_error& e) {
        failed = std::string_view(e.what()).find(needle) != std::string_view::npos;
    }
    assert(failed);
}

template<class T>
void expect_parse_error_for(std::string_view yaml, std::string_view first, std::string_view second) {
    bool failed = false;
    try {
        T value{};
        contract::adapters::yaml::reader<> in(contract::io::window_input{yaml});
        in >> value;
    } catch (const contract::adapters::yaml::parse_error& e) {
        const std::string_view message = e.what();
        failed = message.find(first) != std::string_view::npos &&
                 message.find(second) != std::string_view::npos;
    } catch (const std::runtime_error& e) {
        const std::string_view message = e.what();
        failed = message.find(first) != std::string_view::npos &&
                 message.find(second) != std::string_view::npos;
    }
    assert(failed);
}

template<class T>
void expect_value_parse_error(std::string_view yaml, std::string_view needle) {
    bool failed = false;
    try {
        T value{};
        contract::adapters::yaml::reader<> in(contract::io::window_input{yaml});
        in >> value;
    } catch (const contract::adapters::yaml::parse_error& e) {
        failed = std::string_view(e.what()).find(needle) != std::string_view::npos;
    } catch (const std::runtime_error& e) {
        failed = std::string_view(e.what()).find(needle) != std::string_view::npos;
    }
    assert(failed);
}

} // namespace

int main() {
    const std::string input =
        "service: billing\n"
        "enabled: true\n"
        "limits:\n"
        "  max_items: 7\n"
        "  ratio: 1.5\n"
        "tags:\n"
        "  - payments\n"
        "  - audit\n"
        "ports:\n"
        "  - 8080\n"
        "  - 9090\n"
        "stage:\n"
        "  - 3\n"
        "  - \"initial\"\n"
        "owner: \"ops:team\"\n";

    Config cfg{};
    contract::adapters::yaml::reader<> in(contract::io::window_input{input});
    in >> cfg;

    assert(cfg.service == "billing");
    assert(cfg.enabled);
    assert(cfg.limits.max_items == 7);
    assert(cfg.limits.ratio == 1.5);
    assert((cfg.tags == std::vector<std::string>{"payments", "audit"}));
    assert((cfg.ports == std::vector<std::uint32_t>{8080, 9090}));
    assert(std::get<0>(cfg.stage) == 3);
    assert(std::get<1>(cfg.stage) == "initial");
    assert(!cfg.note.has_value());
    assert(cfg.owner == "ops:team");

    const std::string shuffled_input =
        "owner: \"ops:team\"\n"
        "stage:\n"
        "  - 3\n"
        "  - \"initial\"\n"
        "ports:\n"
        "  - 8080\n"
        "  - 9090\n"
        "tags:\n"
        "  - payments\n"
        "  - audit\n"
        "limits:\n"
        "  ratio: 1.5\n"
        "  max_items: 7\n"
        "enabled: true\n"
        "service: billing\n";

    Config shuffled_cfg{};
    contract::adapters::yaml::reader<> shuffled_in(contract::io::window_input{shuffled_input});
    shuffled_in >> shuffled_cfg;

    assert(shuffled_cfg.service == "billing");
    assert(shuffled_cfg.enabled);
    assert(shuffled_cfg.limits.max_items == 7);
    assert(shuffled_cfg.limits.ratio == 1.5);
    assert((shuffled_cfg.tags == std::vector<std::string>{"payments", "audit"}));
    assert((shuffled_cfg.ports == std::vector<std::uint32_t>{8080, 9090}));
    assert(std::get<0>(shuffled_cfg.stage) == 3);
    assert(std::get<1>(shuffled_cfg.stage) == "initial");
    assert(!shuffled_cfg.note.has_value());
    assert(shuffled_cfg.owner == "ops:team");

    const std::string commented_input =
        "service: billing   # service name\n"
        "enabled: true # enable it\n"
        "\n"
        "# comment line\n"
        "limits:\n"
        "  max_items: 7   # items\n"
        "  ratio: 1.5 # ratio\n"
        "tags:\n"
        "  - payments   # first\n"
        "  - audit # second\n"
        "ports:\n"
        "  - 8080 # port\n"
        "  - 9090\n"
        "stage:\n"
        "  - 3\n"
        "  - \"initial:phase #1\" # preserved inside quotes\n"
        "owner: \"ops:team#1\" # stripped after quote\n";

    Config commented_cfg{};
    contract::adapters::yaml::reader<> commented_in(contract::io::window_input{commented_input});
    commented_in >> commented_cfg;

    assert(commented_cfg.service == "billing");
    assert(commented_cfg.enabled);
    assert(commented_cfg.limits.max_items == 7);
    assert(commented_cfg.limits.ratio == 1.5);
    assert((commented_cfg.tags == std::vector<std::string>{"payments", "audit"}));
    assert((commented_cfg.ports == std::vector<std::uint32_t>{8080, 9090}));
    assert(std::get<0>(commented_cfg.stage) == 3);
    assert(std::get<1>(commented_cfg.stage) == "initial:phase #1");
    assert(commented_cfg.owner == "ops:team#1");

    {
        const std::string numeric_input =
            "signed_value: -9223372036854775808\n"
            "unsigned_value: 18446744073709551615\n"
            "ratio: 1e6\n";

        NumericSample numeric{};
        contract::adapters::yaml::reader<> numeric_in(contract::io::window_input{numeric_input});
        numeric_in >> numeric;

        assert(numeric.signed_value == std::numeric_limits<std::int64_t>::min());
        assert(numeric.unsigned_value == std::numeric_limits<std::uint64_t>::max());
        assert(numeric.ratio == 1e6);
    }

    {
        const std::string enum_input =
            "mode: 2\n";

        EnumSample sample{};
        contract::adapters::yaml::reader<> enum_in(contract::io::window_input{enum_input});
        enum_in >> sample;

        assert(sample.mode == Mode::beta);
    }

    expect_parse_error_for<EnumSample>(
        "mode: beta\n",
        "expected enum scalar",
        "field mode (#1)");

    {
        const std::string nested_optional_present =
            "values:\n"
            "  - 7\n"
            "  - 11\n";

        NestedOptional present{};
        contract::adapters::yaml::reader<> present_in(contract::io::window_input{nested_optional_present});
        present_in >> present;

        assert(present.values.has_value());
        assert((present.values == std::vector<std::uint32_t>{7, 11}));
    }

    {
        const std::string nested_optional_empty =
            "values: null\n";

        NestedOptional empty{};
        contract::adapters::yaml::reader<> empty_in(contract::io::window_input{nested_optional_empty});
        empty_in >> empty;

        assert(!empty.values.has_value());
    }

    const std::string crlf_input =
        "service: billing\r\n"
        "enabled: true\r\n"
        "limits:\r\n"
        "  max_items: 7\r\n"
        "  ratio: 1.5\r\n"
        "tags:\r\n"
        "  - payments\r\n"
        "  - audit\r\n"
        "ports:\r\n"
        "  - 8080\r\n"
        "  - 9090\r\n"
        "stage:\r\n"
        "  - 3\r\n"
        "  - \"initial\"\r\n"
        "owner: \"ops:team\"\r\n";

    Config crlf_cfg{};
    contract::adapters::yaml::reader<> crlf_in(contract::io::window_input{crlf_input});
    crlf_in >> crlf_cfg;
    assert(crlf_cfg.service == "billing");
    assert(crlf_cfg.enabled);
    assert(crlf_cfg.owner == "ops:team");

    {
        const auto temp_path = std::filesystem::temp_directory_path() / "contract_yaml_file_input_test.yaml";
        {
            std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);
            assert(file.is_open());
            file << input;
            assert(file.good());
        }

        Config file_cfg{};
        contract::adapters::yaml::reader<contract::io::file_buffer_input> file_in(
            contract::io::file_buffer_input{temp_path.string()});
        file_in >> file_cfg;

        assert(file_cfg.service == "billing");
        assert(file_cfg.enabled);
        assert(file_cfg.owner == "ops:team");
        std::filesystem::remove(temp_path);
    }

    {
        const std::string nested_sequence_input =
            "limits:\n"
            "  -\n"
            "    max_items: 3\n"
            "    ratio: 0.5\n"
            "  -\n"
            "    max_items: 8\n"
            "    ratio: 2.0\n";

        LimitList list{};
        contract::adapters::yaml::reader<> nested_sequence_in(contract::io::window_input{nested_sequence_input});
        nested_sequence_in >> list;

        assert(list.limits.size() == 2);
        assert(list.limits[0].max_items == 3);
        assert(list.limits[0].ratio == 0.5);
        assert(list.limits[1].max_items == 8);
        assert(list.limits[1].ratio == 2.0);
    }

    {
        const std::string nested_vector_input =
            "limits:\n"
            "  -\n"
            "    max_items: 1\n"
            "    ratio: 0.25\n"
            "  -\n"
            "    max_items: 2\n"
            "    ratio: 0.5\n";

        LimitList list{};
        contract::adapters::yaml::reader<> nested_vector_in(contract::io::window_input{nested_vector_input});
        nested_vector_in >> list;

        assert(list.limits.size() == 2);
        assert(list.limits[0].max_items == 1);
        assert(list.limits[0].ratio == 0.25);
        assert(list.limits[1].max_items == 2);
        assert(list.limits[1].ratio == 0.5);
    }

    {
        const std::string deep_tuple_input =
            "stage:\n"
            "  - 1\n"
            "  - \"mid\"\n"
            "  - 3\n";

        DeepTuple tuple{};
        contract::adapters::yaml::reader<> tuple_in(contract::io::window_input{deep_tuple_input});
        tuple_in >> tuple;

        assert(std::get<0>(tuple.stage) == 1);
        assert(std::get<1>(tuple.stage) == "mid");
        assert(std::get<2>(tuple.stage) == 3);
    }

    {
        std::optional<std::uint32_t> value;
        contract::adapters::yaml::reader<> optional_in(contract::io::window_input{"42\n"});
        optional_in >> value;
        assert(value.has_value());
        assert(*value == 42);
    }

    {
        std::optional<std::uint32_t> value = 42;
        contract::adapters::yaml::reader<> optional_in(contract::io::window_input{"null\n"});
        optional_in >> value;
        assert(!value.has_value());
    }

    {
        ChunkWindowInput chunked{input};
        Config chunked_cfg{};
        contract::adapters::yaml::reader<ChunkWindowInput> chunked_in(chunked);
        chunked_in >> chunked_cfg;

        assert(chunked_cfg.service == "billing");
        assert(chunked_cfg.limits.max_items == 7);
        assert((chunked_cfg.tags == std::vector<std::string>{"payments", "audit"}));
        assert(chunked_cfg.owner == "ops:team");
    }

    {
        ChunkWindowInput tiny_chunked{input};
        tiny_chunked.chunk = 1;
        Config tiny_chunked_cfg{};
        contract::adapters::yaml::reader<ChunkWindowInput> tiny_chunked_in(tiny_chunked);
        tiny_chunked_in >> tiny_chunked_cfg;

        assert(tiny_chunked_cfg.service == "billing");
        assert(tiny_chunked_cfg.limits.max_items == 7);
        assert((tiny_chunked_cfg.tags == std::vector<std::string>{"payments", "audit"}));
        assert(tiny_chunked_cfg.owner == "ops:team");
    }

    expect_parse_error(
        "service: billing\n"
        "service: duplicate\n",
        "duplicate key while reading field at line 2 near \"duplicate\"");

    expect_parse_error(
        "service: billing\n"
        "unknown: field\n",
        "unknown key while reading field at line 2 near \"field\"");

    expect_status_error(
        "service: billing\n"
        "unknown: field\n",
        "unknown key");

    expect_parse_error(
        "service billing\n",
        "expected ':' in mapping entry while reading mapping at line 1 near \"service billing\"");

    expect_parse_error(
        "service: billing\n"
        "enabled: yes\n",
        "expected boolean scalar while reading scalar in Config field enabled (#2) [member] at line 2 near \"yes\"");

    expect_parse_error(
        "service: billing\n"
        "limits: 7\n",
        "expected nested mapping for contract object while reading mapping in Config field limits (#3) [member] at line 2 near \"7\"");

    expect_parse_error(
        "service: billing\n"
        "tags:\n"
        "    - payments\n",
        "unexpected indentation while reading sequence in Config field tags (#4) [member] at line 2");

    expect_parse_error(
        "service:\n"
        "\tenabled: true\n",
        "tabs are not supported while reading scanner at line 1");

    expect_value_parse_error<std::uint32_t>(
        "",
        "expected scalar value while reading root at line 1");

    expect_value_parse_error<std::uint32_t>(
        "   \n\t \n",
        "tabs are not supported while reading scanner at line 2");

    expect_value_parse_error<std::string>(
        "\"bad\\x\"\n",
        "invalid string scalar while reading scalar at line 1 near \"\\\"bad\\\\x\\\"\"");

    expect_value_parse_error<std::uint32_t>(
        "12x\n",
        "expected integral scalar while reading scalar at line 1 near \"12x\"");

    expect_parse_error(
        "service: billing\n"
        "enabled: true\n"
        "extra\n",
        "expected ':' in mapping entry while reading mapping at line 2 near \"true\"");

    expect_parse_error(
        "service: \"bad\\x\"\n",
        "near \"\\\"bad\\\\x\\\"\"");

    expect_parse_error(
        "service: billing\n"
        "enabled: true\n"
        "limits:\n"
        "  max_items: 7\n"
        "  ratio: 1.5\n"
        "tags:\n"
        "  - payments\n"
        "  - audit\n"
        "ports:\n"
        "  - 8080\n"
        "  - 9090\n"
        "stage:\n"
        "  - 3\n"
        "  - \"initial\"\n"
        "owner: \"ops:team\"\n"
        "tags:\n"
        "  - other\n",
        "duplicate key while reading field at line 16");

    expect_parse_error(
        "service: billing\n"
        "enabled: true\n"
        "limits:\n"
        "  max_items: 7\n"
        "  ratio: 1.5\n"
        "tags:\n"
        "  - payments\n"
        "  - audit\n"
        "ports:\n"
        "  - 8080\n"
        "  - 9090\n"
        "stage:\n"
        "  - 3\n"
        "  - \"initial\"\n"
        "owner: \"ops:team\"\n"
        "limits:\n"
        "  ratio: 1.5\n",
        "duplicate key while reading field at line 16");

    {
        const std::string long_input = std::string(64, 'x') + "\n";
        std::string long_value{};
        contract::adapters::yaml::reader<> long_in(contract::io::window_input{long_input});
        long_in >> long_value;
        assert(long_value == std::string(64, 'x'));
    }

    {
        const std::string deep_ok =
            "level2:\n"
            "  level3:\n"
            "    level4:\n"
            "      value: 42\n";

        DeepLevel1 value{};
        contract::adapters::yaml::reader<> deep_in(contract::io::window_input{deep_ok});
        deep_in >> value;
        assert(value.level2.level3.level4.value == 42);
    }

    expect_parse_error_for<DeepLevel1>(
        "level2:\n"
        "  level3:\n"
        "    level4:\n"
        "      value: yes\n",
        "expected integral scalar while reading scalar in DeepLevel4 field value (#1) [member] at line 4 near \"yes\"");

    expect_parse_error_for<DeepLevel1>(
        "level2:\n"
        "  level3:\n"
        "    level4:\n"
        "      valuex: 42\n",
        "unknown key while reading field in DeepLevel3 field level4 (#1) [member] at line 4 near \"42\"");

    expect_parse_error_for<DeepLevel1>(
        "level2:\n"
        "  level3:\n"
        "    level4:\n"
        "      value 42\n",
        "expected ':' in mapping entry while reading mapping in DeepLevel3 field level4 (#1) [member] at line 3");

    return 0;
}
