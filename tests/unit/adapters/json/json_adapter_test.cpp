// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json/all.hpp>
#include <contract/contract.hpp>

#include <cassert>
#include <array>
#include <bitset>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

struct JsonValueOnly {
    std::string value;
};

struct JsonFieldAware {
    std::string value;
};

namespace contract::adapters::json {

template<>
struct codec<JsonValueOnly, void> {
    template<class Writer>
    static void write(Writer& out, const JsonValueOnly& value) {
        out.write_string(value.value);
    }
};

template<>
struct codec<JsonFieldAware, void> {
    template<class Writer>
    static void write(Writer& out, const JsonFieldAware& value) {
        out.write_string(value.value);
    }

    template<class Writer, class Field>
    static void write(Writer& out, const Field& field, const JsonFieldAware&) {
        out.write_string(field.name);
    }
};

} // namespace contract::adapters::json

namespace {

template<class T>
void expect_json(const T& value, const std::string& expected) {
    const auto actual = contract::adapters::json::to_string(value);
    assert(actual == expected);
}

template<class T>
void expect_json(const T& value, const contract::adapters::json::options& opt, const std::string& expected) {
    const auto actual = contract::adapters::json::to_string(value, opt);
    assert(actual == expected);
}

struct LogContext {
    std::string service = "billing";
    std::string subsystem = "api";

    CONTRACT(LogContext, (service, 1), (subsystem, 2))
};

struct LogEvent {
    LogContext context{};
    std::string level = "info";
    std::string message = "request_started";
    std::uint32_t request_id = 42;
    std::optional<std::string> trace_id{"abc"};
    std::vector<std::string> tags{"payments", "audit"};
    std::array<std::uint32_t, 3> retry_delays{1, 2, 5};
    std::tuple<std::string, std::uint32_t> stage{"payment", 2};
    std::variant<std::string, std::uint32_t, LogContext> status{LogContext{"gateway", "api"}};
    std::map<std::string, std::string> headers{
        {"content-type", "application/json"},
        {"x-request-id", "abc"},
    };
    std::unordered_map<std::string, std::uint32_t> counters{
        {"retries", 3},
    };
    std::bitset<10> flags{std::string{"1010010110"}};

    CONTRACT(LogEvent,
        (context, 1),
        (level, 2),
        (message, 3),
        (request_id, 4),
        (trace_id, 5),
        (tags, 6),
        (retry_delays, 7),
        (stage, 8),
        (status, 9),
        (headers, 10),
        (counters, 11),
        (flags, 12))
};

enum class Level : std::uint8_t {
    debug = 1,
    info = 2,
};

struct PrimitiveEnvelope {
    bool enabled = true;
    std::nullptr_t nothing = nullptr;
    double ratio = 3.5;
    Level level = Level::info;
    const char* raw = "A \"quote\" and a slash \\";
    std::string_view view = "line1\nline2";
    std::optional<int> missing{};

    CONTRACT(PrimitiveEnvelope,
        (enabled, 1),
        (nothing, 2),
        (ratio, 3),
        (level, 4),
        (raw, 5),
        (view, 6),
        (missing, 7))
};

struct EmptyCollections {
    std::vector<int> values{};
    std::array<int, 0> fixed{};
    int raw[2]{1, 2};
    std::map<std::string, int> mapping{};

    CONTRACT(EmptyCollections,
        (values, 1),
        (fixed, 2),
        (raw, 3),
        (mapping, 4))
};

struct SecurityEnvelope {
    std::string password = "p@ss";
    std::string api_key = "token";
    std::string note = "public";
    std::string sealed = "sealed";
    std::string mixed = "mixed";
    std::string triple = "triple";

    CONTRACT(SecurityEnvelope,
        (password, 1, contract::security::secret()),
        (api_key, 2, contract::security::no_log()),
        (note, 3, contract::security::sensitive()),
        (sealed, 4, contract::security::no_log(), contract::security::secret()),
        (mixed, 5, contract::security::secret(), contract::security::sensitive()),
        (triple, 6,
            contract::security::no_log(),
            contract::security::secret(),
            contract::security::sensitive()))
};

struct ByteLikeEnvelope {
    char packed[8] = {'c', 'o', 'n', 't', 'r', 'a', 'c', 't'};
    char padded[16] = {'h', 'i'};
    std::array<char, 4> zeroed{};
    char embedded_null[8] = {'a', 'b', 'c', '\0', 'd', 'e', 'f', '\0'};
    std::array<unsigned char, 4> hash{0x10, 0x2b, 0x9a, 0x01};
    std::array<std::byte, 2> flags{std::byte{0x01}, std::byte{0xff}};
    char single = 'x';
    signed char signed_single = -5;
    unsigned char unsigned_single = 250;

    CONTRACT(ByteLikeEnvelope,
        (packed, 1),
        (padded, 2),
        (zeroed, 3),
        (embedded_null, 4),
        (hash, 5),
        (flags, 6),
        (single, 7),
        (signed_single, 8),
        (unsigned_single, 9))
};

struct ControlCharRecord {
    // 0x01 is not a named JSON escape (unlike \n/\r/\t/"/\\) - must use \u00NN.
    std::string message = std::string("bad") + '\x01' + "end";

    CONTRACT(ControlCharRecord, (message, 1))
};

struct CodecDispatchEnvelope {
    JsonValueOnly value_only{"plain"};
    JsonFieldAware field_aware{"plain"};

    CONTRACT(CodecDispatchEnvelope,
        (value_only, 1),
        (field_aware, 2))
};

} // namespace

int main() {
    expect_json(JsonFieldAware{"top-level"}, "\"top-level\"");
    expect_json(CodecDispatchEnvelope{},
        "{\"value_only\":\"plain\",\"field_aware\":\"field_aware\"}");

    expect_json(PrimitiveEnvelope{},
        "{\"enabled\":true,"
        "\"nothing\":null,"
        "\"ratio\":3.5,"
        "\"level\":2,"
        "\"raw\":\"A \\\"quote\\\" and a slash \\\\\","
        "\"view\":\"line1\\nline2\","
        "\"missing\":null}");

    expect_json(EmptyCollections{},
        "{\"values\":[],"
        "\"fixed\":[],"
        "\"raw\":[1,2],"
        "\"mapping\":[]}");

    expect_json(SecurityEnvelope{},
        "{\"password\":\"p@ss\","
        "\"api_key\":\"token\","
        "\"note\":\"public\","
        "\"sealed\":\"sealed\","
        "\"mixed\":\"mixed\","
        "\"triple\":\"triple\"}");

    contract::adapters::json::options security_opt{};
    security_opt.no_log = contract::adapters::json::security_mode::omit;
    security_opt.secret = contract::adapters::json::security_mode::redact;
    security_opt.sensitive = contract::adapters::json::security_mode::redact;

    expect_json(SecurityEnvelope{}, security_opt,
        "{\"password\":\"<redacted>\","
        "\"note\":\"<redacted>\","
        "\"mixed\":\"<redacted>\"}");

    LogEvent event;
    expect_json(event,
        "{\"context\":{\"service\":\"billing\",\"subsystem\":\"api\"},"
        "\"level\":\"info\","
        "\"message\":\"request_started\","
        "\"request_id\":42,"
        "\"trace_id\":\"abc\","
        "\"tags\":[\"payments\",\"audit\"],"
        "\"retry_delays\":[1,2,5],"
        "\"stage\":[\"payment\",2],"
        "\"status\":[2,{\"service\":\"gateway\",\"subsystem\":\"api\"}],"
        "\"headers\":[[\"content-type\",\"application/json\"],[\"x-request-id\",\"abc\"]],"
        "\"counters\":[[\"retries\",3]],"
        "\"flags\":\"1010010110\"}");

    // char[N]/std::array<char,N> are text (a type decision, not a content
    // scan - json is used on the hot-path logger, so no per-value inspection
    // is done). Byte-like non-char types (unsigned char, std::byte) are
    // numbers, matching how any other numeric array already looks in json.
    expect_json(ByteLikeEnvelope{},
        "{\"packed\":\"contract\","
        "\"padded\":\"hi\","
        "\"zeroed\":\"\","
        "\"embedded_null\":\"abc\\u0000def\","
        "\"hash\":[16,43,154,1],"
        "\"flags\":[1,255],"
        "\"single\":\"x\","
        "\"signed_single\":-5,"
        "\"unsigned_single\":250}");

    // Regression: control characters outside the named escapes (\n \r \t "
    // \\) must use \u00NN, not \xNN - \xNN is not valid JSON.
    expect_json(ControlCharRecord{}, "{\"message\":\"bad\\u0001end\"}");

    return 0;
}
