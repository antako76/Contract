// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0
//
// Compares CONTRACT's protobuf adapter against the reference libprotobuf
// implementation, using the same measurement discipline established for the
// compact/protobuf comparison: pre-encode every sample once, then time only
// the per-op pack/unpack, building the writer/reader (or, for libprotobuf,
// just calling Serialize/Parse directly) as a plain per-op local rather than
// a NOINLINE-wrapped helper or a value held behind std::optional - both of
// those were measured this session to distort results by de-optimizing the
// inner io state across a message's field reads.
//
// Only built when CONTRACT_BENCH_WITH_PROTOBUF=ON (requires libprotobuf +
// protoc); the main library and its default benchmarks have no dependency
// on real protobuf.

#include "benchmark_base.hpp"
#include "protobuf_reference_messages.pb.h"

#include <contract/contract.hpp>
#include <contract/adapters/protobuf/all.hpp>
#include <contract/io.hpp>

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

#if defined(_MSC_VER)
#    define CONTRACT_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#    define CONTRACT_NOINLINE __attribute__((noinline))
#else
#    define CONTRACT_NOINLINE
#endif

constexpr std::size_t buffer_size = 4096;
int iterations = 100'000;
constexpr int repeats = 7;

volatile std::uint64_t global_sink = 0;

// --- CONTRACT-side message types, mirroring the .proto schema field-for-field ---

struct NumericMessage {
    std::uint64_t id = 0x0123'4567'89ab'cdefull;
    std::uint32_t count = 42;
    double ratio = 3.25;
    bool enabled = true;

    CONTRACT(NumericMessage, (id, 1), (count, 2), (ratio, 3), (enabled, 4))
};

// Every scenario uses 4 varied samples cycled through a randomized order
// (benchmark_base::make_order), matching protobuf_compact_adapter_benchmark.cpp's
// convention: a single repeated sample gives every op identical branches and
// data, which is easier to predict/cache than realistic varied input and
// would inflate both sides' apparent speed similarly, but unevenly.
//
// None of these samples use a field's proto3 default value (0 / false / "" /
// empty) - a default-valued singular field is omitted entirely on the wire
// by real protobuf (proto3's implicit presence), which CONTRACT does not
// replicate (see project memory: not implementing this is deliberate, not a
// gap - it avoids a well-known proto3 schema-evolution hazard). Avoiding
// defaults in the sample data keeps the size comparison clean instead of
// re-demonstrating that already-understood, already-decided difference.
std::vector<NumericMessage> make_numeric_messages() {
    return {
        NumericMessage{0x0123'4567'89ab'cdefull, 1'000'000, 3'250.25, true},
        NumericMessage{0x0123'4567'89ab'cdf0ull, 2'000'000, 6'500.50, true},
        NumericMessage{0x0123'4567'89ab'cdf1ull, 3'000'000, 9'750.75, true},
        NumericMessage{0x0123'4567'89ab'cdf2ull, 4'000'000, 12'250.25, true},
    };
}

struct TextMessage {
    std::string name = "contract protobuf benchmark";

    CONTRACT(TextMessage, (name, 1))
};

std::vector<TextMessage> make_text_messages() {
    return {
        TextMessage{"contract"},
        TextMessage{"protobuf benchmark"},
        TextMessage{"binary vs protobuf"},
        TextMessage{"simple nested record"},
    };
}

struct NestedMessage {
    NumericMessage numeric{};
    TextMessage text{};

    CONTRACT(NestedMessage, (numeric, 1), (text, 2))
};

std::vector<NestedMessage> make_nested_messages() {
    const auto numeric = make_numeric_messages();
    const auto text = make_text_messages();
    return {
        NestedMessage{numeric[0], text[0]},
        NestedMessage{numeric[1], text[1]},
        NestedMessage{numeric[2], text[2]},
        NestedMessage{numeric[3], text[3]},
    };
}

// Vector-holding message types default to an EMPTY vector, not prefilled
// sample data. measure_contract's unpack loop resets `target` between ops
// (protobuf repeated fields append rather than replace, and reused strings
// need their old content cleared - see the reset()/reset_target overloads
// below), and a non-trivial default member initializer here would silently
// measure "rebuild N elements" on every reset in addition to the actual
// parse - exactly the kind of test-shapes-the-measurement trap this session
// already hit once with std::optional/NOINLINE. Sample data comes from the
// make_* factories below instead, used only when building the `samples`
// vector.

struct VectorMessage {
    std::vector<std::uint32_t> values;

    CONTRACT(VectorMessage, (values, 1))
};

std::vector<VectorMessage> make_vector_messages() {
    std::vector<VectorMessage> v(4);
    v[0].values = {1, 2, 3, 4};
    v[1].values = {5, 6, 7, 8};
    v[2].values = {9, 10, 11, 12};
    v[3].values = {13, 14, 15, 16};
    return v;
}

struct VectorMessage25 {
    std::vector<std::uint32_t> values;

    CONTRACT(VectorMessage25, (values, 1))
};

std::vector<VectorMessage25> make_vector_messages_25() {
    std::vector<VectorMessage25> v(4);
    for (std::size_t s = 0; s < v.size(); ++s) {
        v[s].values.reserve(25);
        for (std::uint32_t i = 0; i < 25; ++i) {
            v[s].values.push_back(i + 1 + static_cast<std::uint32_t>(s) * 25);
        }
    }
    return v;
}

struct WideVectorMessage {
    std::vector<std::uint32_t> values;

    CONTRACT(WideVectorMessage, (values, 1))
};

std::vector<WideVectorMessage> make_wide_vector_messages() {
    std::vector<WideVectorMessage> v(4);
    for (std::size_t s = 0; s < v.size(); ++s) {
        v[s].values.reserve(100);
        for (std::uint32_t i = 0; i < 100; ++i) {
            v[s].values.push_back(i + 1 + static_cast<std::uint32_t>(s) * 100);
        }
    }
    return v;
}

struct StringVectorMessage {
    std::vector<std::string> values;

    CONTRACT(StringVectorMessage, (values, 1))
};

std::vector<StringVectorMessage> make_string_vector_messages() {
    std::vector<StringVectorMessage> v(4);
    v[0].values = {"first-tag", "second-tag", "third-tag", "fourth-tag"};
    v[1].values = {"alpha-tag", "beta-tag", "gamma-tag", "delta-tag"};
    v[2].values = {"red-tag", "green-tag", "blue-tag", "yellow-tag"};
    v[3].values = {"north-tag", "south-tag", "east-tag", "west-tag"};
    return v;
}

struct WideStringVectorMessage {
    std::vector<std::string> values;

    CONTRACT(WideStringVectorMessage, (values, 1))
};

std::vector<WideStringVectorMessage> make_wide_string_vector_messages() {
    std::vector<WideStringVectorMessage> v(4);
    static const char* prefixes[4] = {"tag", "label", "marker", "flag"};
    for (std::size_t s = 0; s < v.size(); ++s) {
        v[s].values.reserve(50);
        for (int i = 0; i < 50; ++i) {
            v[s].values.push_back(std::string(prefixes[s]) + "-" + std::to_string(i));
        }
    }
    return v;
}

struct WideRecord {
    std::uint64_t id = 0x1122'3344'5566'7788ull;
    std::int32_t sequence = -17;
    std::uint32_t count = 4200;
    std::int64_t big_flag = -123456789012345LL;
    std::string name = "svc-7";
    std::string description = "primary ingestion worker for the payments pipeline";
    std::string tag = "critical";
    double ratio = 0.918273;
    float weight = 1.5f;
    bool enabled = true;

    CONTRACT(WideRecord,
        (id, 1), (sequence, 2), (count, 3), (big_flag, 4), (name, 5),
        (description, 6), (tag, 7), (ratio, 8), (weight, 9), (enabled, 10))
};

std::vector<WideRecord> make_wide_records() {
    return {
        WideRecord{0x1122'3344'5566'7788ull, -17, 4200, -123456789012345LL,
            "svc-7", "primary ingestion worker for the payments pipeline", "critical",
            0.918273, 1.5f, true},
        WideRecord{0x2233'4455'6677'8899ull, 42, 8100, 987654321098765LL,
            "svc-3", "secondary batch processor for the export pipeline", "normal",
            0.123456, 2.25f, true},
        WideRecord{0x3344'5566'7788'99aaull, -5000, 15, -1LL,
            "svc-9", "tertiary retry handler for the notification pipeline", "low",
            3.14159, 0.5f, true},
        WideRecord{0x4455'6677'8899'aabbull, 100, 999999, 42LL,
            "svc-1", "quaternary audit logger for the compliance pipeline", "high",
            2.71828, 10.0f, true},
    };
}

struct AllStringsMessage {
    std::string first_name;
    std::string last_name;
    std::string email;
    std::string company;
    std::string department;
    std::string notes;

    CONTRACT(AllStringsMessage,
        (first_name, 1), (last_name, 2), (email, 3),
        (company, 4), (department, 5), (notes, 6))
};

std::vector<AllStringsMessage> make_all_strings_messages() {
    return {
        AllStringsMessage{"Alice", "Nguyen", "alice.nguyen@example.com",
            "Acme Corp", "Payments", "Handles chargebacks and disputes"},
        AllStringsMessage{"Bilal", "Khan", "bilal.khan@example.com",
            "Globex", "Platform", "On-call rotation lead for Q3"},
        AllStringsMessage{"Carmen", "Diaz", "carmen.diaz@example.com",
            "Initech", "Security", "Owns the incident response runbook"},
        AllStringsMessage{"Deshi", "Tanaka", "deshi.tanaka@example.com",
            "Umbrella", "Data", "Maintains the ingestion pipeline dashboards"},
    };
}

// int32 underlying type to match how protobuf enums are actually represented
// on the wire (always a varint int32, regardless of the .proto declaration) -
// std::uint8_t would hit protobuf's missing unsigned char scalar codec.
enum class Status : std::int32_t {
    pending = 0,
    active = 1,
    closed = 2,
};

struct AllNumbersMessage {
    std::int32_t a = 0;
    std::uint32_t b = 0;
    std::int64_t c = 0;
    std::uint64_t d = 0;
    float e = 0.0f;
    double f = 0.0;
    bool g = false;
    Status status = Status::pending;

    CONTRACT(AllNumbersMessage,
        (a, 1), (b, 2), (c, 3), (d, 4), (e, 5), (f, 6), (g, 7), (status, 8))
};

std::vector<AllNumbersMessage> make_all_numbers_messages() {
    return {
        AllNumbersMessage{-1, 1, -100, 100, 1.5f, 2.5, true, Status::active},
        AllNumbersMessage{-2'000'000, 2'000'000, -200'000'000'000LL, 200'000'000'000ULL,
            -3.25f, 6.5, true, Status::active},
        AllNumbersMessage{2'147'483'647, 4'000'000'000U, 9'000'000'000'000LL,
            18'000'000'000'000'000'000ULL, 12.75f, -12.75, true, Status::closed},
        AllNumbersMessage{7, 3, 9, 4, 100.0f, -0.0009765625, true, Status::active},
    };
}

struct BytesMessage {
    char payload[32] = {};

    CONTRACT(BytesMessage, (payload, 1))
};

std::vector<BytesMessage> make_bytes_messages() {
    std::vector<BytesMessage> v(4);
    const char* content[4] = {"session-token-abc123", "req-9f2e", "x", "a-somewhat-longer-payload-value"};
    for (std::size_t s = 0; s < v.size(); ++s) {
        const std::string_view text{content[s]};
        std::memcpy(v[s].payload, text.data(), text.size());
    }
    return v;
}

// A flat record of 25 individual scalar fields, as opposed to one field
// holding a 25-element vector - exercises per-field dispatch overhead (25
// separate tag/codec calls) rather than one repeated-field loop.
struct Int25Message {
    std::uint32_t f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0;
    std::uint32_t f6 = 0, f7 = 0, f8 = 0, f9 = 0, f10 = 0;
    std::uint32_t f11 = 0, f12 = 0, f13 = 0, f14 = 0, f15 = 0;
    std::uint32_t f16 = 0, f17 = 0, f18 = 0, f19 = 0, f20 = 0;
    std::uint32_t f21 = 0, f22 = 0, f23 = 0, f24 = 0, f25 = 0;

    CONTRACT(Int25Message,
        (f1, 1), (f2, 2), (f3, 3), (f4, 4), (f5, 5),
        (f6, 6), (f7, 7), (f8, 8), (f9, 9), (f10, 10),
        (f11, 11), (f12, 12), (f13, 13), (f14, 14), (f15, 15),
        (f16, 16), (f17, 17), (f18, 18), (f19, 19), (f20, 20),
        (f21, 21), (f22, 22), (f23, 23), (f24, 24), (f25, 25))
};

Int25Message make_int25(std::uint32_t base) {
    Int25Message m;
    m.f1 = base + 1; m.f2 = base + 2; m.f3 = base + 3; m.f4 = base + 4; m.f5 = base + 5;
    m.f6 = base + 6; m.f7 = base + 7; m.f8 = base + 8; m.f9 = base + 9; m.f10 = base + 10;
    m.f11 = base + 11; m.f12 = base + 12; m.f13 = base + 13; m.f14 = base + 14; m.f15 = base + 15;
    m.f16 = base + 16; m.f17 = base + 17; m.f18 = base + 18; m.f19 = base + 19; m.f20 = base + 20;
    m.f21 = base + 21; m.f22 = base + 22; m.f23 = base + 23; m.f24 = base + 24; m.f25 = base + 25;
    return m;
}

std::vector<Int25Message> make_int25_messages() {
    return {make_int25(0), make_int25(1000), make_int25(1'000'000), make_int25(50)};
}

// Same shape as Int25Message but with strings - isolates whether the
// per-field dispatch overhead is specific to the varint/int write path or a
// more general many-same-type-fields cost.
struct Str25Message {
    std::string f1, f2, f3, f4, f5;
    std::string f6, f7, f8, f9, f10;
    std::string f11, f12, f13, f14, f15;
    std::string f16, f17, f18, f19, f20;
    std::string f21, f22, f23, f24, f25;

    CONTRACT(Str25Message,
        (f1, 1), (f2, 2), (f3, 3), (f4, 4), (f5, 5),
        (f6, 6), (f7, 7), (f8, 8), (f9, 9), (f10, 10),
        (f11, 11), (f12, 12), (f13, 13), (f14, 14), (f15, 15),
        (f16, 16), (f17, 17), (f18, 18), (f19, 19), (f20, 20),
        (f21, 21), (f22, 22), (f23, 23), (f24, 24), (f25, 25))
};

Str25Message make_str25(const std::string& prefix) {
    Str25Message m;
    m.f1 = prefix + "-01"; m.f2 = prefix + "-02"; m.f3 = prefix + "-03"; m.f4 = prefix + "-04"; m.f5 = prefix + "-05";
    m.f6 = prefix + "-06"; m.f7 = prefix + "-07"; m.f8 = prefix + "-08"; m.f9 = prefix + "-09"; m.f10 = prefix + "-10";
    m.f11 = prefix + "-11"; m.f12 = prefix + "-12"; m.f13 = prefix + "-13"; m.f14 = prefix + "-14"; m.f15 = prefix + "-15";
    m.f16 = prefix + "-16"; m.f17 = prefix + "-17"; m.f18 = prefix + "-18"; m.f19 = prefix + "-19"; m.f20 = prefix + "-20";
    m.f21 = prefix + "-21"; m.f22 = prefix + "-22"; m.f23 = prefix + "-23"; m.f24 = prefix + "-24"; m.f25 = prefix + "-25";
    return m;
}

std::vector<Str25Message> make_str25_messages() {
    return {make_str25("alpha"), make_str25("bravo"), make_str25("charlie"), make_str25("delta")};
}

// --- comparison result row ---

struct stat {
    std::size_t size = 0;
    double pack = 0.0;
    double unpack = 0.0;
};

struct row {
    std::string name;
    stat contract_stat;
    stat proto_stat;
};

template<class Writer>
Writer make_contract_writer(unsigned char* data, std::size_t size) {
    return Writer{contract::io::window_output{data, size}};
}

// CONTRACT protobuf side: same pattern as protobuf_compact_adapter_benchmark.cpp.
// Reset is a customization point: `target = Value{}` would discard whatever
// buffers `target` had allocated (a fresh Value{} is move-assigned over it),
// unlike libprotobuf's Clear() which keeps existing capacity. The no-Reset
// overload below picks reset_target, which calls a type's reset() overload
// (see above) when one exists and falls back to `target = Value{}` only for
// types with no heap-owning fields to preserve capacity of.
template<class Value, class Reset>
CONTRACT_NOINLINE stat measure_contract(const std::vector<Value>& samples, const std::vector<std::size_t>& order, Reset reset) {
    using Writer = contract::adapters::protobuf::writer<contract::io::window_output>;
    using Reader = contract::adapters::protobuf::reader<contract::io::window_input>;

    const std::size_t n = samples.size();

    std::vector<unsigned char> buffer(n * buffer_size);
    std::vector<std::pair<std::size_t, std::size_t>> frames(n);
    std::size_t offset = 0;
    for (std::size_t k = 0; k < n; ++k) {
        auto w = make_contract_writer<Writer>(buffer.data() + offset, buffer.size() - offset);
        w << samples[k];
        frames[k] = {offset, w.position()};
        offset += w.position();
    }
    const std::size_t size_per_value = n ? (offset / n) : offset;

    const auto pick = [&](int i) -> std::size_t {
        if (order.empty()) {
            return static_cast<std::size_t>(i) % n;
        }
        return order[static_cast<std::size_t>(i) % order.size()] % n;
    };

    std::vector<unsigned char> scratch(buffer_size);
    const double pack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        auto out = make_contract_writer<Writer>(scratch.data(), scratch.size());
        out << samples[pick(i)];
        global_sink += out.position();
    });

    Value target{};
    const double unpack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto& f = frames[pick(i)];
        // Repeated fields append rather than replace - reset per op, same
        // fix required for the compact/protobuf benchmark this session.
        reset(target);
        auto in = Reader{contract::io::window_input{buffer.data() + f.first, f.second}};
        in >> target;
        global_sink += f.second;
    });

    return {size_per_value, pack, unpack};
}

template<class Value>
CONTRACT_NOINLINE stat measure_contract(const std::vector<Value>& samples, const std::vector<std::size_t>& order) {
    return measure_contract(samples, order, [](Value& target) { reset_target(target); });
}

// Real libprotobuf side: no writer/reader wrapper exists in its API at all,
// so there is no NOINLINE/optional pitfall to avoid here - Serialize/Parse
// are called directly on a plain per-op local target message.
template<class ProtoMsg>
CONTRACT_NOINLINE stat measure_proto(const std::vector<ProtoMsg>& samples, const std::vector<std::size_t>& order) {
    const std::size_t n = samples.size();

    std::vector<unsigned char> buffer(n * buffer_size);
    std::vector<std::pair<std::size_t, std::size_t>> frames(n);
    std::size_t offset = 0;
    for (std::size_t k = 0; k < n; ++k) {
        const std::size_t len = samples[k].ByteSizeLong();
        samples[k].SerializeToArray(buffer.data() + offset, static_cast<int>(len));
        frames[k] = {offset, len};
        offset += len;
    }
    const std::size_t size_per_value = n ? (offset / n) : offset;

    const auto pick = [&](int i) -> std::size_t {
        if (order.empty()) {
            return static_cast<std::size_t>(i) % n;
        }
        return order[static_cast<std::size_t>(i) % order.size()] % n;
    };

    std::vector<unsigned char> scratch(buffer_size);
    const double pack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto& msg = samples[pick(i)];
        const std::size_t len = msg.ByteSizeLong();
        msg.SerializeToArray(scratch.data(), static_cast<int>(len));
        global_sink += len;
    });

    ProtoMsg target;
    const double unpack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto& f = frames[pick(i)];
        target.Clear();
        target.ParseFromArray(buffer.data() + f.first, static_cast<int>(f.second));
        global_sink += f.second;
    });

    return {size_per_value, pack, unpack};
}

contract_bench::NumericMessage to_proto(const NumericMessage& v) {
    contract_bench::NumericMessage m;
    m.set_id(v.id);
    m.set_count(v.count);
    m.set_ratio(v.ratio);
    m.set_enabled(v.enabled);
    return m;
}

contract_bench::TextMessage to_proto(const TextMessage& v) {
    contract_bench::TextMessage m;
    m.set_name(v.name);
    return m;
}

contract_bench::NestedMessage to_proto(const NestedMessage& v) {
    contract_bench::NestedMessage m;
    *m.mutable_numeric() = to_proto(v.numeric);
    *m.mutable_text() = to_proto(v.text);
    return m;
}

contract_bench::VectorMessage to_proto(const VectorMessage& v) {
    contract_bench::VectorMessage m;
    for (auto value : v.values) {
        m.add_values(value);
    }
    return m;
}

contract_bench::VectorMessage25 to_proto(const VectorMessage25& v) {
    contract_bench::VectorMessage25 m;
    for (auto value : v.values) {
        m.add_values(value);
    }
    return m;
}

contract_bench::WideVectorMessage to_proto(const WideVectorMessage& v) {
    contract_bench::WideVectorMessage m;
    for (auto value : v.values) {
        m.add_values(value);
    }
    return m;
}

contract_bench::StringVectorMessage to_proto(const StringVectorMessage& v) {
    contract_bench::StringVectorMessage m;
    for (const auto& value : v.values) {
        m.add_values(value);
    }
    return m;
}

contract_bench::WideStringVectorMessage to_proto(const WideStringVectorMessage& v) {
    contract_bench::WideStringVectorMessage m;
    for (const auto& value : v.values) {
        m.add_values(value);
    }
    return m;
}

contract_bench::WideRecord to_proto(const WideRecord& v) {
    contract_bench::WideRecord m;
    m.set_id(v.id);
    m.set_sequence(v.sequence);
    m.set_count(v.count);
    m.set_big_flag(v.big_flag);
    m.set_name(v.name);
    m.set_description(v.description);
    m.set_tag(v.tag);
    m.set_ratio(v.ratio);
    m.set_weight(v.weight);
    m.set_enabled(v.enabled);
    return m;
}

contract_bench::AllStringsMessage to_proto(const AllStringsMessage& v) {
    contract_bench::AllStringsMessage m;
    m.set_first_name(v.first_name);
    m.set_last_name(v.last_name);
    m.set_email(v.email);
    m.set_company(v.company);
    m.set_department(v.department);
    m.set_notes(v.notes);
    return m;
}

contract_bench::AllNumbersMessage to_proto(const AllNumbersMessage& v) {
    contract_bench::AllNumbersMessage m;
    m.set_a(v.a);
    m.set_b(v.b);
    m.set_c(v.c);
    m.set_d(v.d);
    m.set_e(v.e);
    m.set_f(v.f);
    m.set_g(v.g);
    m.set_status(static_cast<contract_bench::Status>(static_cast<int>(v.status)));
    return m;
}

contract_bench::BytesMessage to_proto(const BytesMessage& v) {
    contract_bench::BytesMessage m;
    // Mirror compact/protobuf's own trailing-zero trim so the wire size
    // comparison stays apples-to-apples (see contract::adapters::base::trim_trailing_zeros).
    std::size_t len = sizeof(v.payload);
    while (len > 0 && v.payload[len - 1] == '\0') {
        --len;
    }
    m.set_payload(v.payload, len);
    return m;
}

contract_bench::Int25Message to_proto(const Int25Message& v) {
    contract_bench::Int25Message m;
    m.set_f1(v.f1); m.set_f2(v.f2); m.set_f3(v.f3); m.set_f4(v.f4); m.set_f5(v.f5);
    m.set_f6(v.f6); m.set_f7(v.f7); m.set_f8(v.f8); m.set_f9(v.f9); m.set_f10(v.f10);
    m.set_f11(v.f11); m.set_f12(v.f12); m.set_f13(v.f13); m.set_f14(v.f14); m.set_f15(v.f15);
    m.set_f16(v.f16); m.set_f17(v.f17); m.set_f18(v.f18); m.set_f19(v.f19); m.set_f20(v.f20);
    m.set_f21(v.f21); m.set_f22(v.f22); m.set_f23(v.f23); m.set_f24(v.f24); m.set_f25(v.f25);
    return m;
}

contract_bench::Str25Message to_proto(const Str25Message& v) {
    contract_bench::Str25Message m;
    m.set_f1(v.f1); m.set_f2(v.f2); m.set_f3(v.f3); m.set_f4(v.f4); m.set_f5(v.f5);
    m.set_f6(v.f6); m.set_f7(v.f7); m.set_f8(v.f8); m.set_f9(v.f9); m.set_f10(v.f10);
    m.set_f11(v.f11); m.set_f12(v.f12); m.set_f13(v.f13); m.set_f14(v.f14); m.set_f15(v.f15);
    m.set_f16(v.f16); m.set_f17(v.f17); m.set_f18(v.f18); m.set_f19(v.f19); m.set_f20(v.f20);
    m.set_f21(v.f21); m.set_f22(v.f22); m.set_f23(v.f23); m.set_f24(v.f24); m.set_f25(v.f25);
    return m;
}

// --- reset helpers: bring a persistent unpack target back to a clean state
// the way protobuf's own Clear() does - clearing existing heap-owning
// fields in place, not tearing the whole value down and rebuilding it via
// `target = Value{}`. The latter frees whatever capacity `target` already
// had, forcing a fresh allocation on the very next parse; `container.clear()`
// keeps that capacity, so repeated same-shaped parses stop allocating after
// the first couple of iterations - matching the reuse pattern this
// benchmark's own median_per_op loop actually exercises. ---

void reset(VectorMessage& t) { t.values.clear(); }
void reset(VectorMessage25& t) { t.values.clear(); }
void reset(WideVectorMessage& t) { t.values.clear(); }
void reset(StringVectorMessage& t) { t.values.clear(); }
void reset(WideStringVectorMessage& t) { t.values.clear(); }

void reset(TextMessage& t) { t.name.clear(); }
void reset(NestedMessage& t) { reset(t.text); }

void reset(WideRecord& t) {
    t.name.clear();
    t.description.clear();
    t.tag.clear();
}

void reset(AllStringsMessage& t) {
    t.first_name.clear();
    t.last_name.clear();
    t.email.clear();
    t.company.clear();
    t.department.clear();
    t.notes.clear();
}

void reset(Str25Message& t) {
    t.f1.clear(); t.f2.clear(); t.f3.clear(); t.f4.clear(); t.f5.clear();
    t.f6.clear(); t.f7.clear(); t.f8.clear(); t.f9.clear(); t.f10.clear();
    t.f11.clear(); t.f12.clear(); t.f13.clear(); t.f14.clear(); t.f15.clear();
    t.f16.clear(); t.f17.clear(); t.f18.clear(); t.f19.clear(); t.f20.clear();
    t.f21.clear(); t.f22.clear(); t.f23.clear(); t.f24.clear(); t.f25.clear();
}

// Types with no heap-owning fields (NumericMessage, AllNumbersMessage,
// BytesMessage) have no reset() overload above and don't need one -
// `target = Value{}` is already free for them, so the fallback below is
// exactly as cheap as a hand-written reset would be.
template<class Value>
concept has_reset_overload = requires(Value& v) { reset(v); };

template<class Value>
void reset_target(Value& target) {
    if constexpr (has_reset_overload<Value>) {
        reset(target);
    } else {
        target = Value{};
    }
}

template<class Value>
auto to_proto_vector(const std::vector<Value>& values) {
    std::vector<decltype(to_proto(values.front()))> result;
    result.reserve(values.size());
    for (const auto& v : values) {
        result.push_back(to_proto(v));
    }
    return result;
}

template<class Value, class ProtoMsg>
CONTRACT_NOINLINE row benchmark_row(std::string name, const std::vector<Value>& samples, const std::vector<ProtoMsg>& proto_samples) {
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), samples.size());
    const auto cs = measure_contract(samples, order);
    const auto ps = measure_proto(proto_samples, order);
    return {std::move(name), cs, ps};
}

void print_row(const row& r) {
    const double pack_ratio = r.proto_stat.pack != 0.0 ? r.contract_stat.pack / r.proto_stat.pack : 0.0;
    const double unpack_ratio = r.proto_stat.unpack != 0.0 ? r.contract_stat.unpack / r.proto_stat.unpack : 0.0;

    std::cout << std::left << std::setw(20) << r.name
              << std::right
              << " size(c/p)=" << std::setw(4) << r.contract_stat.size << "/" << std::setw(4) << r.proto_stat.size
              << "  pack(c/p)=" << std::setw(8) << std::fixed << std::setprecision(2) << r.contract_stat.pack
              << "/" << std::setw(8) << r.proto_stat.pack << "ns"
              << "  x" << std::setw(5) << std::setprecision(2) << pack_ratio
              << "  unpack(c/p)=" << std::setw(8) << r.contract_stat.unpack
              << "/" << std::setw(8) << r.proto_stat.unpack << "ns"
              << "  x" << std::setw(5) << unpack_ratio
              << "\n";
}

} // namespace

int main(int argc, char** argv) {
    iterations = benchmark_base::parse_iterations_arg(argc, argv, iterations);

    std::cout << "CONTRACT protobuf adapter vs real libprotobuf (protobuf "
              << GOOGLE_PROTOBUF_VERSION << ")\n";
    std::cout << "iterations=" << iterations << " repeats=" << repeats << "\n";
    std::cout << "size in bytes; pack/unpack in ns/op (median of " << repeats << "); "
                 "x column = contract/protobuf ratio (>1 means contract is slower)\n\n";

    {
        const auto samples = make_numeric_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("numeric", samples, proto_samples));
    }
    {
        const auto samples = make_text_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("text", samples, proto_samples));
    }
    {
        const auto samples = make_nested_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("nested", samples, proto_samples));
    }
    {
        const auto samples = make_vector_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("vector[4]", samples, proto_samples));
    }
    {
        const auto samples = make_vector_messages_25();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("vector[25]", samples, proto_samples));
    }
    {
        const auto samples = make_wide_vector_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("vector[100]", samples, proto_samples));
    }
    {
        const auto samples = make_wide_records();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("wide[10 fields]", samples, proto_samples));
    }
    {
        const auto samples = make_string_vector_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("string_vector[4]", samples, proto_samples));
    }
    {
        const auto samples = make_wide_string_vector_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("string_vector[50]", samples, proto_samples));
    }
    {
        const auto samples = make_all_strings_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("all_strings[6]", samples, proto_samples));
    }
    {
        const auto samples = make_all_numbers_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("all_numbers[8]", samples, proto_samples));
    }
    {
        const auto samples = make_bytes_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("bytes[32]", samples, proto_samples));
    }
    {
        const auto samples = make_int25_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("int25[25 fields]", samples, proto_samples));
    }
    {
        const auto samples = make_str25_messages();
        const auto proto_samples = to_proto_vector(samples);
        print_row(benchmark_row("str25[25 fields]", samples, proto_samples));
    }

    return 0;
}
