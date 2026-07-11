#include "benchmark_base.hpp"

#include <contract/contract.hpp>
#include <contract/adapters/compact/all.hpp>
#include <contract/adapters/binary/all.hpp>
#include <contract/adapters/protobuf/all.hpp>
#include <contract/io.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace {

#if defined(_MSC_VER)
#    define CONTRACT_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#    define CONTRACT_NOINLINE __attribute__((noinline))
#else
#    define CONTRACT_NOINLINE
#endif

constexpr std::size_t buffer_size = 1024;
int iterations = 100'000;
constexpr int repeats = 7;
volatile std::uint64_t global_sink = 0;
volatile std::uint64_t buffer_version_sink = 0;

struct NumericMessage {
    std::uint64_t id = 0x0123'4567'89ab'cdefull;
    std::uint32_t count = 42;
    double ratio = 3.25;
    bool enabled = true;

    CONTRACT(NumericMessage, (id, 1), (count, 2), (ratio, 3), (enabled, 4))
};

struct SmallMessage {
    std::uint32_t a = 1;
    std::uint32_t b = 2;
    std::uint32_t c = 3;
    std::uint32_t d = 4;

    CONTRACT(SmallMessage, (a, 1), (b, 2), (c, 3), (d, 4))
};

struct OneFieldMessage {
    std::uint32_t value = 42;

    CONTRACT(OneFieldMessage, (value, 1))
};

struct TextMessage {
    std::string name = "contract protobuf benchmark";

    CONTRACT(TextMessage, (name, 1))
};

struct ViewMessage {
    std::string_view name = "contract protobuf benchmark";

    CONTRACT(ViewMessage, (name, 1))
};

struct BoolMessage {
    bool value = true;

    CONTRACT(BoolMessage, (value, 1))
};

struct SignedMessage {
    std::int64_t value = -42;

    CONTRACT(SignedMessage, (value, 1))
};

struct UnsignedMessage {
    std::uint64_t value = 42;

    CONTRACT(UnsignedMessage, (value, 1))
};

struct DoubleMessage {
    double value = 3.25;

    CONTRACT(DoubleMessage, (value, 1))
};

struct TinyMessage {
    std::uint32_t value = 42;

    CONTRACT(TinyMessage, (value, 1))
};

struct NestedMessage {
    NumericMessage numeric{};
    TextMessage text{};

    CONTRACT(NestedMessage, (numeric, 1), (text, 2))
};

std::uint64_t checksum(const NumericMessage& value);
std::uint64_t checksum(const SmallMessage& value);
std::uint64_t checksum(const OneFieldMessage& value);
std::uint64_t checksum(const TextMessage& value);
std::uint64_t checksum(const ViewMessage& value);
std::uint64_t checksum(const BoolMessage& value);
std::uint64_t checksum(const SignedMessage& value);
std::uint64_t checksum(const UnsignedMessage& value);
std::uint64_t checksum(std::uint32_t value);
std::uint64_t checksum(const DoubleMessage& value);
std::uint64_t checksum(const TinyMessage& value);
std::uint64_t checksum(const NestedMessage& value);

template<class Value>
struct encoded_blob {
    std::array<unsigned char, buffer_size> storage{};
    std::size_t size = 0;
};

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_binary(const Value& value) {
    encoded_blob<Value> blob{};
    contract::adapters::binary::writer<contract::io::checked_output> out{
        contract::io::checked_output{blob.storage.data(), blob.storage.size()}};
    out << value;
    blob.size = static_cast<std::size_t>(out.current() - blob.storage.data());
    return blob;
}

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_protobuf(const Value& value) {
    encoded_blob<Value> blob{};
    contract::io::window_output sink{blob.storage.data(), blob.storage.size()};
    contract::adapters::protobuf::writer<contract::io::window_output> out{sink};
    out << value;
    blob.size = out.position();
    return blob;
}

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_compact(const Value& value) {
    encoded_blob<Value> blob{};
    contract::io::window_output sink{blob.storage.data(), blob.storage.size()};
    contract::adapters::compact::writer<> out{sink};
    out << value;
    blob.size = out.position();
    return blob;
}

template<class Value>
CONTRACT_NOINLINE void decode_binary(Value& value, const encoded_blob<Value>& blob) {
    contract::adapters::binary::reader<contract::io::checked_input> in{
        contract::io::checked_input{blob.storage.data(), blob.size}};
    in >> value;
}

template<class Value>
CONTRACT_NOINLINE void decode_protobuf(Value& value, const encoded_blob<Value>& blob) {
    contract::io::window_input source{blob.storage.data(), blob.size};
    contract::adapters::protobuf::reader<contract::io::window_input> in{source};
    in >> value;
}

template<class Value>
CONTRACT_NOINLINE void decode_compact(Value& value, const encoded_blob<Value>& blob) {
    contract::io::window_input source{blob.storage.data(), blob.size};
    contract::adapters::compact::reader<contract::io::window_input> in{source};
    in >> value;
}

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_binary_scalar(const Value& value) {
    encoded_blob<Value> blob{};
    contract::adapters::binary::writer<contract::io::checked_output> out{
        contract::io::checked_output{blob.storage.data(), blob.storage.size()}};
    out << value;
    blob.size = static_cast<std::size_t>(out.current() - blob.storage.data());
    return blob;
}

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_protobuf_scalar(const Value& value) {
    encoded_blob<Value> blob{};
    contract::io::window_output sink{blob.storage.data(), blob.storage.size()};
    contract::adapters::protobuf::writer<contract::io::window_output> out{sink};
    out.write_varint(static_cast<std::uint64_t>(value));
    blob.size = out.position();
    return blob;
}

template<class Value>
CONTRACT_NOINLINE encoded_blob<Value> encode_compact_scalar(const Value& value) {
    encoded_blob<Value> blob{};
    contract::io::window_output sink{blob.storage.data(), blob.storage.size()};
    contract::adapters::compact::writer<> out{sink};
    out.write_uint(static_cast<std::uint64_t>(value));
    blob.size = out.position();
    return blob;
}

template<class Value>
CONTRACT_NOINLINE void decode_binary_scalar(Value& value, const encoded_blob<Value>& blob) {
    contract::adapters::binary::reader<contract::io::checked_input> in{
        contract::io::checked_input{blob.storage.data(), blob.size}};
    in >> value;
}

template<class Value>
CONTRACT_NOINLINE void decode_protobuf_scalar(Value& value, const encoded_blob<Value>& blob) {
    contract::io::window_input source{blob.storage.data(), blob.size};
    contract::adapters::protobuf::reader<contract::io::window_input> in{source};
    std::uint64_t raw = 0;
    if (in.read_varint(raw) == contract::adapters::base::status::error) {
        throw std::runtime_error(in.error_message());
    }
    value = static_cast<Value>(raw);
}

template<class Value>
CONTRACT_NOINLINE void decode_compact_scalar(Value& value, const encoded_blob<Value>& blob) {
    contract::io::window_input source{blob.storage.data(), blob.size};
    contract::adapters::compact::reader<contract::io::window_input> in{source};
    std::uint64_t raw = 0;
    if (in.read_uint(raw) == contract::adapters::base::status::error) {
        throw std::runtime_error(in.error_message());
    }
    value = static_cast<Value>(raw);
}

template<class Samples>
CONTRACT_NOINLINE double measure_binary_pack(const Samples& samples, const std::vector<std::size_t>& order) {
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& value = samples[slot];
        auto blob = encode_binary(value);
        global_sink += blob.size;
        buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
    });
}

template<class Samples>
CONTRACT_NOINLINE double measure_protobuf_pack(const Samples& samples, const std::vector<std::size_t>& order) {
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& value = samples[slot];
        auto blob = encode_protobuf(value);
        global_sink += blob.size;
        buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
    });
}

template<class Samples>
CONTRACT_NOINLINE double measure_compact_pack(const Samples& samples, const std::vector<std::size_t>& order) {
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& value = samples[slot];
        auto blob = encode_compact(value);
        global_sink += blob.size;
        buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
    });
}

template<class Samples>
CONTRACT_NOINLINE double measure_binary_unpack(const Samples& samples, const std::vector<std::size_t>& order) {
    std::vector<encoded_blob<typename Samples::value_type>> blobs;
    blobs.reserve(samples.size());
    for (const auto& sample : samples) {
        blobs.push_back(encode_binary(sample));
    }

    typename Samples::value_type target{};
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& blob = blobs[slot];
        target = typename Samples::value_type{};
        decode_binary(target, blob);
        global_sink += checksum(target);
    });
}

template<class Samples>
CONTRACT_NOINLINE double measure_protobuf_unpack(const Samples& samples, const std::vector<std::size_t>& order) {
    std::vector<encoded_blob<typename Samples::value_type>> blobs;
    blobs.reserve(samples.size());
    for (const auto& sample : samples) {
        blobs.push_back(encode_protobuf(sample));
    }

    typename Samples::value_type target{};
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& blob = blobs[slot];
        target = typename Samples::value_type{};
        decode_protobuf(target, blob);
        global_sink += checksum(target);
    });
}

template<class Samples>
CONTRACT_NOINLINE double measure_compact_unpack(const Samples& samples, const std::vector<std::size_t>& order) {
    std::vector<encoded_blob<typename Samples::value_type>> blobs;
    blobs.reserve(samples.size());
    for (const auto& sample : samples) {
        blobs.push_back(encode_compact(sample));
    }

    typename Samples::value_type target{};
    return benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto slot = order[static_cast<std::size_t>(i)];
        const auto& blob = blobs[slot];
        target = typename Samples::value_type{};
        decode_compact(target, blob);
        global_sink += checksum(target);
    });
}

struct row {
    std::string name;
    std::size_t binary_size = 0;
    std::size_t protobuf_size = 0;
    std::size_t compact_size = 0;
    double binary_pack = 0.0;
    double protobuf_pack = 0.0;
    double compact_pack = 0.0;
    double binary_unpack = 0.0;
    double protobuf_unpack = 0.0;
    double compact_unpack = 0.0;
};

template<class Samples>
CONTRACT_NOINLINE row benchmark_scalar_u32(std::string name, const Samples& samples) {
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), samples.size());
    std::vector<encoded_blob<typename Samples::value_type>> binary_blobs;
    std::vector<encoded_blob<typename Samples::value_type>> protobuf_blobs;
    std::vector<encoded_blob<typename Samples::value_type>> compact_blobs;
    binary_blobs.reserve(samples.size());
    protobuf_blobs.reserve(samples.size());
    compact_blobs.reserve(samples.size());
    for (const auto& sample : samples) {
        binary_blobs.push_back(encode_binary_scalar(sample));
        protobuf_blobs.push_back(encode_protobuf_scalar(sample));
        compact_blobs.push_back(encode_compact_scalar(sample));
    }

    return {
        std::move(name),
        binary_blobs.front().size,
        protobuf_blobs.front().size,
        compact_blobs.front().size,
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& value = samples[slot];
            auto blob = encode_binary_scalar(value);
            global_sink += blob.size;
            buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
        }),
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& value = samples[slot];
            auto blob = encode_protobuf_scalar(value);
            global_sink += blob.size;
            buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
        }),
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& value = samples[slot];
            auto blob = encode_compact_scalar(value);
            global_sink += blob.size;
            buffer_version_sink = (buffer_version_sink << 1) ^ benchmark_base::fold_bytes(blob.storage.data(), blob.size);
        }),
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& blob = binary_blobs[slot];
            std::uint32_t target = 0;
            decode_binary_scalar(target, blob);
            global_sink += target;
        }),
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& blob = protobuf_blobs[slot];
            std::uint32_t target = 0;
            decode_protobuf_scalar(target, blob);
            global_sink += target;
        }),
        benchmark_base::median_per_op(repeats, iterations, [&](int i) {
            const auto slot = order[static_cast<std::size_t>(i)];
            const auto& blob = compact_blobs[slot];
            std::uint32_t target = 0;
            decode_compact_scalar(target, blob);
            global_sink += target;
        }),
    };
}

void print_row(const row& r);

struct row_entry {
    std::string name;
    std::function<row()> build;
};

struct cli_config {
    std::optional<std::size_t> row_index;
    std::optional<std::string> row_name;
    bool list_rows = false;
};

void print_usage(std::ostream& out, const char* argv0) {
    out << "usage: " << argv0
        << " [--iterations N]"
        << " [--list-rows]"
        << " [--row-index N | --row NAME]\n";
}

cli_config parse_cli(int argc, char** argv) {
    cli_config config;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--help" || arg == "-h") {
            print_usage(std::cout, argv[0]);
            std::exit(0);
        } else if (arg == "--row-index" && i + 1 < argc) {
            config.row_index = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (arg.rfind("--row-index=", 0) == 0) {
            config.row_index = static_cast<std::size_t>(std::stoull(std::string(arg.substr(12))));
        } else if (arg == "--row" && i + 1 < argc) {
            config.row_name = std::string(argv[++i]);
        } else if (arg.rfind("--row=", 0) == 0) {
            config.row_name = std::string(arg.substr(6));
        } else if (arg == "--list-rows") {
            config.list_rows = true;
        }
    }
    return config;
}

void print_row_list(const std::vector<row_entry>& entries, std::ostream& out) {
    for (std::size_t i = 0; i < entries.size(); ++i) {
        out << (i + 1) << ": " << entries[i].name << "\n";
    }
}

void prefix_row_names(std::vector<row>& rows, std::size_t first_index = 1) {
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i].name = std::to_string(first_index + i) + ": " + rows[i].name;
    }
}

void print_rows(std::vector<row> rows, std::size_t first_index = 1) {
    prefix_row_names(rows, first_index);
    for (const auto& r : rows) {
        print_row(r);
    }
}

template<class Samples>
CONTRACT_NOINLINE row benchmark_one(std::string name, const Samples& samples) {
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), samples.size());
    const auto binary_blob0 = encode_binary(samples[0]);
    const auto protobuf_blob0 = encode_protobuf(samples[0]);
    const auto compact_blob0 = encode_compact(samples[0]);
    return {
        std::move(name),
        binary_blob0.size,
        protobuf_blob0.size,
        compact_blob0.size,
        measure_binary_pack(samples, order),
        measure_protobuf_pack(samples, order),
        measure_compact_pack(samples, order),
        measure_binary_unpack(samples, order),
        measure_protobuf_unpack(samples, order),
        measure_compact_unpack(samples, order),
    };
}

std::uint64_t checksum(const NumericMessage& value) {
    return value.id + value.count + static_cast<std::uint64_t>(value.ratio * 100.0) + static_cast<std::uint64_t>(value.enabled);
}

std::uint64_t checksum(const SmallMessage& value) {
    return value.a + value.b + value.c + value.d;
}

std::uint64_t checksum(const OneFieldMessage& value) {
    return value.value;
}

std::uint64_t checksum(const TextMessage& value) {
    return static_cast<std::uint64_t>(value.name.size());
}

std::uint64_t checksum(const ViewMessage& value) {
    return static_cast<std::uint64_t>(value.name.size());
}

std::uint64_t checksum(const BoolMessage& value) {
    return value.value ? 1u : 0u;
}

std::uint64_t checksum(const SignedMessage& value) {
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(value.value));
}

std::uint64_t checksum(const UnsignedMessage& value) {
    return value.value;
}

std::uint64_t checksum(std::uint32_t value) {
    return value;
}

std::uint64_t checksum(const DoubleMessage& value) {
    return static_cast<std::uint64_t>(value.value * 100.0);
}

std::uint64_t checksum(const TinyMessage& value) {
    return value.value;
}

std::uint64_t checksum(const NestedMessage& value) {
    return checksum(value.numeric) + checksum(value.text);
}

void print_row(const row& r) {
    const auto protobuf_pack_ratio = r.protobuf_pack / r.binary_pack;
    const auto compact_pack_ratio = r.compact_pack / r.binary_pack;
    const auto compact_to_protobuf_pack_ratio = r.compact_pack / r.protobuf_pack;
    const auto protobuf_unpack_ratio = r.protobuf_unpack / r.binary_unpack;
    const auto compact_unpack_ratio = r.compact_unpack / r.binary_unpack;
    const auto compact_to_protobuf_unpack_ratio = r.compact_unpack / r.protobuf_unpack;

    std::cout << std::left
              << std::setw(14) << r.name
              << std::right
              << std::setw(12) << r.binary_size
              << std::setw(14) << r.protobuf_size
              << std::setw(14) << r.compact_size
              << std::setw(14) << std::fixed << std::setprecision(2) << r.binary_pack
              << std::setw(14) << r.protobuf_pack
              << std::setw(14) << r.compact_pack
              << std::setw(10) << protobuf_pack_ratio
              << std::setw(10) << compact_pack_ratio
              << std::setw(10) << compact_to_protobuf_pack_ratio
              << std::setw(14) << r.binary_unpack
              << std::setw(14) << r.protobuf_unpack
              << std::setw(14) << r.compact_unpack
              << std::setw(10) << protobuf_unpack_ratio
              << std::setw(10) << compact_unpack_ratio
              << std::setw(10) << compact_to_protobuf_unpack_ratio
              << "\n";
}

} // namespace

int main(int argc, char** argv) {
    const auto cli = parse_cli(argc, argv);
    iterations = benchmark_base::parse_iterations_arg(argc, argv, iterations);
    const std::array<ViewMessage, 4> view{
        ViewMessage{"contract"},
        ViewMessage{"protobuf benchmark"},
        ViewMessage{"binary vs protobuf"},
        ViewMessage{"simple nested record"},
    };
    const std::array<BoolMessage, 4> boolean{
        BoolMessage{true},
        BoolMessage{false},
        BoolMessage{true},
        BoolMessage{false},
    };
    const std::array<SignedMessage, 4> signed_int{
        SignedMessage{-1'000'000},
        SignedMessage{-42'000'000},
        SignedMessage{0},
        SignedMessage{9'876'543'210ll},
    };
    const std::array<UnsignedMessage, 4> unsigned_int{
        UnsignedMessage{0},
        UnsignedMessage{1'000'000},
        UnsignedMessage{42'000'000},
        UnsignedMessage{9'876'543'210ull},
    };
    const std::array<DoubleMessage, 4> floating{
        DoubleMessage{0.0},
        DoubleMessage{1'250.5},
        DoubleMessage{3'250.75},
        DoubleMessage{123'456.5},
    };
    const std::array<TinyMessage, 4> tiny{
        TinyMessage{0},
        TinyMessage{1'000},
        TinyMessage{42'000},
        TinyMessage{1'234'567},
    };
    std::vector<std::uint32_t> raw_u32;
    raw_u32.reserve(100);
    for (std::uint32_t value = 1; value <= 100; ++value) {
        raw_u32.push_back(value);
    }
    std::vector<UnsignedMessage> unsigned_range;
    unsigned_range.reserve(1000);
    for (std::uint32_t value = 1; value <= 1000; ++value) {
        unsigned_range.push_back(UnsignedMessage{value});
    }
    const std::array<SmallMessage, 4> small{
        SmallMessage{1000, 2000, 3000, 4000},
        SmallMessage{5000, 6000, 7000, 8000},
        SmallMessage{9000, 10'000, 11'000, 12'000},
        SmallMessage{13'000, 14'000, 15'000, 16'000},
    };
    const std::array<OneFieldMessage, 4> one_field{
        OneFieldMessage{1'000},
        OneFieldMessage{2'000},
        OneFieldMessage{3'000},
        OneFieldMessage{4'000},
    };
    const std::array<NumericMessage, 4> numeric{
        NumericMessage{0x0123'4567'89ab'cdefull, 1'000'000, 3'250.25, true},
        NumericMessage{0x0123'4567'89ab'cdf0ull, 2'000'000, 6'500.50, false},
        NumericMessage{0x0123'4567'89ab'cdf1ull, 3'000'000, 9'750.75, true},
        NumericMessage{0x0123'4567'89ab'cdf2ull, 4'000'000, 12'250.25, false},
    };
    const std::array<TextMessage, 4> text{
        TextMessage{"contract"},
        TextMessage{"protobuf benchmark"},
        TextMessage{"binary vs protobuf"},
        TextMessage{"simple nested record"},
    };
    const std::array<NestedMessage, 4> nested{
        NestedMessage{numeric[0], text[0]},
        NestedMessage{numeric[1], text[1]},
        NestedMessage{numeric[2], text[2]},
        NestedMessage{numeric[3], text[3]},
    };

    std::vector<row_entry> entries;
    entries.reserve(13);
    entries.push_back({"view", [&] { return benchmark_one("view", view); }});
    entries.push_back({"bool", [&] { return benchmark_one("bool", boolean); }});
    entries.push_back({"i64", [&] { return benchmark_one("i64", signed_int); }});
    entries.push_back({"u64", [&] { return benchmark_one("u64", unsigned_int); }});
    entries.push_back({"double", [&] { return benchmark_one("double", floating); }});
    entries.push_back({"tiny", [&] { return benchmark_one("tiny", tiny); }});
    entries.push_back({"u32 raw[1..100]", [&] { return benchmark_scalar_u32("u32 raw[1..100]", raw_u32); }});
    entries.push_back({"u32[1..1000]", [&] { return benchmark_one("u32[1..1000]", unsigned_range); }});
    entries.push_back({"small", [&] { return benchmark_one("small", small); }});
    entries.push_back({"one", [&] { return benchmark_one("one", one_field); }});
    entries.push_back({"numeric", [&] { return benchmark_one("numeric", numeric); }});
    entries.push_back({"string", [&] { return benchmark_one("string", text); }});
    entries.push_back({"nested", [&] { return benchmark_one("nested", nested); }});

    std::cout << "iterations: " << iterations
              << " repeats: " << repeats
              << " metric: ns/op\n\n";

    if (cli.list_rows) {
        print_row_list(entries, std::cout);
        return 0;
    }

    std::cout << std::left
              << std::setw(14) << "scenario"
              << std::right
              << std::setw(12) << "bin size"
              << std::setw(14) << "proto size"
              << std::setw(14) << "compact sz"
              << std::setw(14) << "bin pack"
              << std::setw(14) << "proto pack"
              << std::setw(14) << "compact pk"
              << std::setw(10) << "p/bin"
              << std::setw(10) << "c/bin"
              << std::setw(10) << "c/p"
              << std::setw(14) << "bin unpack"
              << std::setw(14) << "proto unpack"
              << std::setw(14) << "compact un"
              << std::setw(10) << "p/bin"
              << std::setw(10) << "c/bin"
              << std::setw(10) << "c/p"
              << "\n";

    if (cli.row_index.has_value() || cli.row_name.has_value()) {
        std::vector<row> selected;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto index = i + 1;
            const bool match_index = cli.row_index.has_value() && *cli.row_index == index;
            const bool match_name = cli.row_name.has_value() && entries[i].name == *cli.row_name;
            if (match_index || match_name) {
                selected.push_back(entries[i].build());
            }
        }

        if (selected.empty()) {
            std::cerr << "no benchmark row matched";
            if (cli.row_index.has_value()) {
                std::cerr << " --row-index " << *cli.row_index;
            }
            if (cli.row_name.has_value()) {
                std::cerr << " --row " << *cli.row_name;
            }
            std::cerr << "\n";
            return 1;
        }

        const auto first_index = cli.row_index.has_value() ? *cli.row_index : 1;
        print_rows(std::move(selected), first_index);
    } else {
        std::vector<row> rows;
        rows.reserve(entries.size());
        for (auto& entry : entries) {
            rows.push_back(entry.build());
        }
        print_rows(std::move(rows));
    }

    std::cout << "\nsink: " << global_sink << "\n";
    std::cout << "buffer version: 0x" << std::hex << std::setw(0) << buffer_version_sink << std::dec << "\n";
}
