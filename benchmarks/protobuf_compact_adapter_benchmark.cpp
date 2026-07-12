#include "benchmark_base.hpp"

#include <contract/contract.hpp>
#include <contract/adapters/compact/all.hpp>
#include <contract/adapters/protobuf/all.hpp>
#include <contract/io.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <bitset>
#include <map>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>
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
volatile std::uint64_t buffer_version_sink = 0;

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

struct SmallMessage {
    std::uint32_t a = 1;
    std::uint32_t b = 2;
    std::uint32_t c = 3;
    std::uint32_t d = 4;

    CONTRACT(SmallMessage, (a, 1), (b, 2), (c, 3), (d, 4))
};

struct OneFieldMessage {
    std::uint64_t value = 1'000;
    CONTRACT(OneFieldMessage, (value, 1))
};

struct TextMessage {
    std::string name = "contract protobuf benchmark";
    CONTRACT(TextMessage, (name, 1))
};

struct NumericMessage {
    std::uint64_t id = 0x0123'4567'89ab'cdefull;
    std::uint32_t count = 42;
    double ratio = 3.25;
    bool enabled = true;

    CONTRACT(NumericMessage, (id, 1), (count, 2), (ratio, 3), (enabled, 4))
};

struct NestedMessage {
    NumericMessage numeric{};
    TextMessage text{};

    CONTRACT(NestedMessage, (numeric, 1), (text, 2))
};

// A more realistic nesting shape than `nested` above: several levels deep,
// 5-7 fields per level, rather than one level with a 2-4 field submessage -
// closer to a typical multi-level DTO than a minimal toy struct.
struct DeepLevel3 {
    std::uint32_t a = 1;
    std::uint32_t b = 2;
    std::uint32_t c = 3;
    std::uint32_t d = 4;
    std::uint32_t e = 5;
    std::uint32_t f = 6;
    std::uint32_t g = 7;

    CONTRACT(DeepLevel3, (a, 1), (b, 2), (c, 3), (d, 4), (e, 5), (f, 6), (g, 7))
};

struct DeepLevel2 {
    std::uint32_t p1 = 11;
    std::uint32_t p2 = 12;
    std::uint32_t p3 = 13;
    std::uint32_t p4 = 14;
    std::uint32_t p5 = 15;
    DeepLevel3 inner{};

    CONTRACT(DeepLevel2, (p1, 1), (p2, 2), (p3, 3), (p4, 4), (p5, 5), (inner, 6))
};

struct DeepLevel1 {
    std::uint32_t q1 = 21;
    std::uint32_t q2 = 22;
    std::uint32_t q3 = 23;
    std::uint32_t q4 = 24;
    std::uint32_t q5 = 25;
    std::uint32_t q6 = 26;
    DeepLevel2 inner{};

    CONTRACT(DeepLevel1, (q1, 1), (q2, 2), (q3, 3), (q4, 4), (q5, 5), (q6, 6), (inner, 7))
};

struct DeepNestedMessage {
    std::uint32_t r1 = 31;
    std::uint32_t r2 = 32;
    std::uint32_t r3 = 33;
    std::uint32_t r4 = 34;
    std::uint32_t r5 = 35;
    DeepLevel1 inner{};
    TextMessage text{};

    CONTRACT(DeepNestedMessage, (r1, 1), (r2, 2), (r3, 3), (r4, 4), (r5, 5), (inner, 6), (text, 7))
};

struct OptionalMessage {
    std::optional<std::uint32_t> count{42};

    CONTRACT(OptionalMessage, (count, 1))
};

struct VectorMessage {
    std::vector<std::uint32_t> values{1, 2, 3, 4};

    CONTRACT(VectorMessage, (values, 1))
};

template<std::size_t N>
struct ArrayMessage {
    std::array<std::uint32_t, N> values{};

    CONTRACT(ArrayMessage, (values, 1))
};

struct TupleMessage {
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        7,
        "tuple",
        {1, 2, 3}
    };

    CONTRACT(TupleMessage, (payload, 1))
};

struct VariantMessage {
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        std::string("variant-value")
    };

    CONTRACT(VariantMessage, (payload, 1))
};

struct Bitset10Message {
    std::bitset<10> flags{0x155};

    CONTRACT(Bitset10Message, (flags, 1))
};

struct Bitset100Message {
    std::bitset<100> flags{0x5555};

    CONTRACT(Bitset100Message, (flags, 1))
};

struct MapMessage {
    std::map<std::uint32_t, std::string> labels{{1, "one"}, {3, "three"}};

    CONTRACT(MapMessage, (labels, 1))
};

struct UnorderedMapMessage {
    std::unordered_map<std::uint32_t, std::string> labels{{3, "three"}, {1, "one"}};

    CONTRACT(UnorderedMapMessage, (labels, 1))
};

struct WideMessage {
    std::uint32_t f01 = 1;
    std::uint32_t f02 = 2;
    std::uint32_t f03 = 3;
    std::uint32_t f04 = 4;
    std::uint32_t f05 = 5;
    std::uint32_t f06 = 6;
    std::uint32_t f07 = 7;
    std::uint32_t f08 = 8;
    std::uint32_t f09 = 9;
    std::uint32_t f10 = 10;
    std::uint32_t f11 = 11;
    std::uint32_t f12 = 12;
    std::uint32_t f13 = 13;
    std::uint32_t f14 = 14;
    std::uint32_t f15 = 15;
    std::uint32_t f16 = 16;

    CONTRACT(WideMessage,
        (f01, 1), (f02, 2), (f03, 3), (f04, 4),
        (f05, 5), (f06, 6), (f07, 7), (f08, 8),
        (f09, 9), (f10, 10), (f11, 11), (f12, 12),
        (f13, 13), (f14, 14), (f15, 15), (f16, 16)
    )
};

template<class T, class = void>
struct has_reserve : std::false_type {};

template<class T>
struct has_reserve<T, std::void_t<decltype(std::declval<T&>().reserve(std::size_t{}))>> : std::true_type {};

template<class Map>
Map make_label_map(std::size_t count, std::uint32_t start, std::size_t text_size) {
    Map labels;
    if constexpr (has_reserve<Map>::value) {
        labels.reserve(count);
    }

    for (std::size_t i = 0; i < count; ++i) {
        const auto key = start + static_cast<std::uint32_t>(i);
        auto text = std::string{"label-"} + std::to_string(key);
        if (text.size() < text_size) {
            text.resize(text_size, static_cast<char>('a' + (key % 26)));
        }
        labels.emplace(key, std::move(text));
    }

    return labels;
}

template<std::size_t N>
std::array<std::uint32_t, N> make_u32_array(std::uint32_t start) {
    std::array<std::uint32_t, N> values{};
    for (std::size_t i = 0; i < N; ++i) {
        values[i] = start + static_cast<std::uint32_t>(i);
    }
    return values;
}

std::vector<std::uint32_t> make_u32_vector(std::size_t count, std::uint32_t start) {
    std::vector<std::uint32_t> values;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        values.push_back(start + static_cast<std::uint32_t>(i));
    }
    return values;
}

template<std::size_t N>
std::bitset<N> make_bitset_pattern(bool start_one) {
    std::bitset<N> value{};
    for (std::size_t i = 0; i < N; ++i) {
        if (((i % 2) == 0) == start_one) {
            value.set(i);
        }
    }
    return value;
}

struct row {
    std::string name;
    std::size_t protobuf_size = 0;
    std::size_t compact_size = 0;
    double protobuf_pack = 0.0;
    double compact_pack = 0.0;
    double protobuf_unpack = 0.0;
    double compact_unpack = 0.0;
};

// Unified measurement model
// ------------------------------------------------------------------
// Every sample is pre-encoded once into a buffer with recorded (offset,len)
// frames. Each timed op builds the reader/writer as a plain local (cheap and
// fully optimizable in the inlined timing loop) over one sample's bytes and
// encodes/decodes it. The reader decodes from a bounded window, which is
// exactly one message for the non-self-delimiting protobuf format. The value
// being NOINLINE-wrapped or held behind std::optional both de-optimize the
// inner io state across a message's field reads, so we avoid both.
struct adapter_stat {
    std::size_t size = 0;
    double pack = 0.0;
    double unpack = 0.0;
};

template<class Samples, class MakeWriter, class EncodeOne, class MakeReader, class DecodeOne>
CONTRACT_NOINLINE adapter_stat measure_adapter(
    const Samples& samples,
    const std::vector<std::size_t>& order,
    MakeWriter make_writer,
    EncodeOne encode_one,
    MakeReader make_reader,
    DecodeOne decode_one)
{
    using Value = typename Samples::value_type;
    const std::size_t n = samples.size();

    // Pre-encode every sample once, contiguously, recording per-sample frames.
    std::vector<unsigned char> buffer(n * buffer_size);
    std::vector<std::pair<std::size_t, std::size_t>> frames(n);
    std::size_t offset = 0;
    for (std::size_t k = 0; k < n; ++k) {
        auto w = make_writer(buffer.data() + offset, buffer.size() - offset);
        encode_one(w, samples[k]);
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

    // PACK: writer is a plain local built per op (cheap, fully optimized in the
    // inlined loop). Holding it behind std::optional adds indirection that stops
    // the compiler optimizing io-state across the encode's inner calls.
    std::vector<unsigned char> scratch(buffer_size);
    const double pack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        auto out = make_writer(scratch.data(), scratch.size());
        encode_one(out, samples[pick(i)]);
        global_sink += out.position();
    });

    // UNPACK: reader is a plain local over each sample's bounded window (one
    // message for protobuf), built per op and fully optimized in the inlined
    // loop; std::optional indirection would de-optimize the inner io state.
    Value target{};
    const double unpack = benchmark_base::median_per_op(repeats, iterations, [&](int i) {
        const auto& f = frames[pick(i)];
        // Decode into a fresh target each op: protobuf repeated fields append
        // rather than replace, so a reused target would accumulate unboundedly.
        target = Value{};
        auto in = make_reader(buffer.data() + f.first, f.second);
        decode_one(in, target);
        if constexpr (std::is_arithmetic_v<Value>) {
            global_sink += static_cast<std::uint64_t>(target);
        } else {
            global_sink += checksum(target);
        }
    });

    return {size_per_value, pack, unpack};
}

template<class Samples>
CONTRACT_NOINLINE row benchmark_row(std::string name, const Samples& samples) {
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), samples.size());
    const auto pb = measure_adapter(
        samples, order,
        [](unsigned char* d, std::size_t c) {
            return contract::adapters::protobuf::writer<contract::io::window_output>{
                contract::io::window_output{d, c}};
        },
        [](auto& w, const auto& v) { w << v; },
        [](const unsigned char* d, std::size_t s) {
            return contract::adapters::protobuf::reader<contract::io::window_input>{
                contract::io::window_input{d, s}};
        },
        [](auto& r, auto& t) { r >> t; });
    const auto cp = measure_adapter(
        samples, order,
        [](unsigned char* d, std::size_t c) {
            return contract::adapters::compact::writer<>{contract::io::window_output{d, c}};
        },
        [](auto& w, const auto& v) { w << v; },
        [](const unsigned char* d, std::size_t s) {
            return contract::adapters::compact::reader<contract::io::window_input>{
                contract::io::window_input{d, s}};
        },
        [](auto& r, auto& t) { r >> t; });
    return {std::move(name), pb.size, cp.size, pb.pack, cp.pack, pb.unpack, cp.unpack};
}

template<class Samples>
CONTRACT_NOINLINE row benchmark_scalar_row(std::string name, const Samples& samples) {
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), samples.size());
    const auto pb = measure_adapter(
        samples, order,
        [](unsigned char* d, std::size_t c) {
            return contract::adapters::protobuf::writer<contract::io::window_output>{
                contract::io::window_output{d, c}};
        },
        [](auto& w, const auto& v) { w.write_varint(static_cast<std::uint64_t>(v)); },
        [](const unsigned char* d, std::size_t s) {
            return contract::adapters::protobuf::reader<contract::io::window_input>{
                contract::io::window_input{d, s}};
        },
        [](auto& r, auto& t) {
            std::uint64_t raw = 0;
            r.read_varint(raw);
            t = static_cast<std::remove_reference_t<decltype(t)>>(raw);
        });
    const auto cp = measure_adapter(
        samples, order,
        [](unsigned char* d, std::size_t c) {
            return contract::adapters::compact::writer<>{contract::io::window_output{d, c}};
        },
        [](auto& w, const auto& v) { w.write_uint(static_cast<std::uint64_t>(v)); },
        [](const unsigned char* d, std::size_t s) {
            return contract::adapters::compact::reader<contract::io::window_input>{
                contract::io::window_input{d, s}};
        },
        [](auto& r, auto& t) {
            std::uint64_t raw = 0;
            r.read_uint(raw);
            t = static_cast<std::remove_reference_t<decltype(t)>>(raw);
        });
    return {std::move(name), pb.size, cp.size, pb.pack, cp.pack, pb.unpack, cp.unpack};
}

std::string row_name(std::string_view base, std::size_t size) {
    std::string name;
    name.reserve(base.size() + 24);
    name.append(base);
    name.push_back('[');
    name.append(std::to_string(size));
    name.push_back(']');
    return name;
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

std::uint64_t checksum(const DoubleMessage& value) {
    return static_cast<std::uint64_t>(value.value * 100.0);
}

std::uint64_t checksum(const TinyMessage& value) {
    return value.value;
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

std::uint64_t checksum(const std::string& value) {
    return static_cast<std::uint64_t>(value.size());
}

std::uint64_t checksum(const NumericMessage& value) {
    return value.id + value.count + static_cast<std::uint64_t>(value.ratio * 100.0) + static_cast<std::uint64_t>(value.enabled);
}

std::uint64_t checksum(const NestedMessage& value) {
    return checksum(value.numeric) + checksum(value.text);
}

std::uint64_t checksum(const DeepLevel3& value) {
    return value.a + value.b + value.c + value.d + value.e + value.f + value.g;
}

std::uint64_t checksum(const DeepLevel2& value) {
    return value.p1 + value.p2 + value.p3 + value.p4 + value.p5 + checksum(value.inner);
}

std::uint64_t checksum(const DeepLevel1& value) {
    return value.q1 + value.q2 + value.q3 + value.q4 + value.q5 + value.q6 + checksum(value.inner);
}

std::uint64_t checksum(const DeepNestedMessage& value) {
    return value.r1 + value.r2 + value.r3 + value.r4 + value.r5 +
        checksum(value.inner) + checksum(value.text);
}

std::uint64_t checksum(const WideMessage& value) {
    return value.f01 + value.f02 + value.f03 + value.f04 +
        value.f05 + value.f06 + value.f07 + value.f08 +
        value.f09 + value.f10 + value.f11 + value.f12 +
        value.f13 + value.f14 + value.f15 + value.f16;
}

template<class T>
std::uint64_t checksum(const std::optional<T>& value);

template<class T>
std::uint64_t checksum(const std::vector<T>& value);

template<class T, std::size_t N>
std::uint64_t checksum(const std::array<T, N>& value);

template<class... Ts>
std::uint64_t checksum(const std::tuple<Ts...>& value);

template<class... Ts>
std::uint64_t checksum(const std::variant<Ts...>& value);

template<std::size_t N>
std::uint64_t checksum(const std::bitset<N>& value);

template<class K, class V, class... Rest>
std::uint64_t checksum(const std::map<K, V, Rest...>& value);

template<class K, class V, class... Rest>
std::uint64_t checksum(const std::unordered_map<K, V, Rest...>& value);

std::uint64_t checksum(std::uint32_t value);
std::uint64_t checksum(const std::string& value);

std::uint64_t checksum(const OptionalMessage& value) {
    return checksum(value.count);
}

std::uint64_t checksum(const VectorMessage& value) {
    return checksum(value.values);
}

template<std::size_t N>
std::uint64_t checksum(const ArrayMessage<N>& value) {
    return checksum(value.values);
}

std::uint64_t checksum(const TupleMessage& value) {
    return checksum(value.payload);
}

std::uint64_t checksum(const VariantMessage& value) {
    return checksum(value.payload);
}

std::uint64_t checksum(const Bitset10Message& value) {
    return checksum(value.flags);
}

std::uint64_t checksum(const Bitset100Message& value) {
    return checksum(value.flags);
}

std::uint64_t checksum(const MapMessage& value) {
    return checksum(value.labels);
}

std::uint64_t checksum(const UnorderedMapMessage& value) {
    return checksum(value.labels);
}

template<class T>
std::uint64_t checksum(const std::optional<T>& value) {
    return value.has_value() ? 1u + checksum(*value) : 0u;
}

template<class T>
std::uint64_t checksum(const std::vector<T>& value) {
    std::uint64_t sum = 0;
    for (const auto& item : value) {
        sum += checksum(item);
    }
    return sum;
}

template<class T, std::size_t N>
std::uint64_t checksum(const std::array<T, N>& value) {
    std::uint64_t sum = 0;
    for (const auto& item : value) {
        sum += checksum(item);
    }
    return sum;
}

template<class... Ts>
std::uint64_t checksum(const std::tuple<Ts...>& value) {
    std::uint64_t sum = 0;
    std::apply([&](const auto&... items) {
        ((sum += checksum(items)), ...);
    }, value);
    return sum;
}

template<class... Ts>
std::uint64_t checksum(const std::variant<Ts...>& value) {
    return std::visit([](const auto& item) -> std::uint64_t {
        return checksum(item);
    }, value);
}

template<std::size_t N>
std::uint64_t checksum(const std::bitset<N>& value) {
    return static_cast<std::uint64_t>(value.count());
}

template<class K, class V, class... Rest>
std::uint64_t checksum(const std::map<K, V, Rest...>& value) {
    std::uint64_t sum = 0;
    for (const auto& [key, mapped] : value) {
        sum += checksum(key);
        sum += checksum(mapped);
    }
    return sum;
}

template<class K, class V, class... Rest>
std::uint64_t checksum(const std::unordered_map<K, V, Rest...>& value) {
    std::uint64_t sum = 0;
    for (const auto& [key, mapped] : value) {
        sum += checksum(key);
        sum += checksum(mapped);
    }
    return sum;
}

std::uint64_t checksum(std::uint32_t value) {
    return value;
}

void print_row(const row& r) {
    const auto pack_ratio = r.compact_pack / r.protobuf_pack;
    const auto unpack_ratio = r.compact_unpack / r.protobuf_unpack;

    std::cout << std::left
              << std::setw(18) << r.name
              << std::right
              << std::setw(14) << r.protobuf_size
              << std::setw(14) << r.compact_size
              << std::setw(14) << std::fixed << std::setprecision(2) << r.protobuf_pack
              << std::setw(14) << r.compact_pack
              << std::setw(10) << pack_ratio
              << std::setw(14) << r.protobuf_unpack
              << std::setw(14) << r.compact_unpack
              << std::setw(10) << unpack_ratio
              << "\n";
}

struct row_entry {
    std::string name;
    std::function<row()> build;
};

struct row_sort_key {
    std::string base;
    std::size_t size = 0;
    std::string suffix;
};

row_sort_key parse_row_sort_key(const std::string& name) {
    const auto open = name.find('[');
    const auto close = name.rfind(']');

    row_sort_key key;
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        key.base = name;
        return key;
    }

    key.base = name.substr(0, open);
    key.suffix = name.substr(open + 1, close - open - 1);

    std::size_t pos = 0;
    while (pos < key.suffix.size() && !std::isdigit(static_cast<unsigned char>(key.suffix[pos]))) {
        ++pos;
    }
    if (pos < key.suffix.size()) {
        key.size = static_cast<std::size_t>(std::stoull(key.suffix.substr(pos)));
    }

    return key;
}

struct cli_config {
    bool list_rows = false;
    std::optional<std::size_t> row_index;
    std::optional<std::string> row_name;
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

} // namespace

int main(int argc, char** argv) {
    const auto cli = parse_cli(argc, argv);
    iterations = benchmark_base::parse_iterations_arg(argc, argv, iterations);

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
    const std::array<SmallMessage, 4> small{
        SmallMessage{1'000, 2'000, 3'000, 4'000},
        SmallMessage{5'000, 6'000, 7'000, 8'000},
        SmallMessage{9'000, 10'000, 11'000, 12'000},
        SmallMessage{13'000, 14'000, 15'000, 16'000},
    };
    const std::array<OneFieldMessage, 4> one_field{
        OneFieldMessage{1'000},
        OneFieldMessage{2'000},
        OneFieldMessage{3'000},
        OneFieldMessage{4'000},
    };
    const std::array<TextMessage, 4> text{
        TextMessage{"contract"},
        TextMessage{"protobuf benchmark"},
        TextMessage{"binary vs protobuf"},
        TextMessage{"simple nested record"},
    };
    const std::array<NumericMessage, 4> numeric{
        NumericMessage{0x0123'4567'89ab'cdefull, 1'000'000, 3'250.25, true},
        NumericMessage{0x0123'4567'89ab'cdf0ull, 2'000'000, 6'500.50, false},
        NumericMessage{0x0123'4567'89ab'cdf1ull, 3'000'000, 9'750.75, true},
        NumericMessage{0x0123'4567'89ab'cdf2ull, 4'000'000, 12'250.25, false},
    };
    const std::array<NestedMessage, 4> nested{
        NestedMessage{numeric[0], text[0]},
        NestedMessage{numeric[1], text[1]},
        NestedMessage{numeric[2], text[2]},
        NestedMessage{numeric[3], text[3]},
    };
    const std::array<DeepNestedMessage, 4> deep_nested{
        DeepNestedMessage{31, 32, 33, 34, 35, DeepLevel1{}, text[0]},
        DeepNestedMessage{41, 42, 43, 44, 45, DeepLevel1{21, 22, 23, 24, 25, 26,
            DeepLevel2{11, 12, 13, 14, 15, DeepLevel3{101, 102, 103, 104, 105, 106, 107}}}, text[1]},
        DeepNestedMessage{51, 52, 53, 54, 55, DeepLevel1{31, 32, 33, 34, 35, 36,
            DeepLevel2{21, 22, 23, 24, 25, DeepLevel3{201, 202, 203, 204, 205, 206, 207}}}, text[2]},
        DeepNestedMessage{61, 62, 63, 64, 65, DeepLevel1{41, 42, 43, 44, 45, 46,
            DeepLevel2{31, 32, 33, 34, 35, DeepLevel3{301, 302, 303, 304, 305, 306, 307}}}, text[3]},
    };
    const std::array<OptionalMessage, 4> optional{
        OptionalMessage{std::optional<std::uint32_t>{42}},
        OptionalMessage{std::optional<std::uint32_t>{}},
        OptionalMessage{std::optional<std::uint32_t>{1'000}},
        OptionalMessage{std::optional<std::uint32_t>{7}},
    };
    const std::array<VectorMessage, 4> vector_small{
        VectorMessage{std::vector<std::uint32_t>{1, 2, 3, 4}},
        VectorMessage{std::vector<std::uint32_t>{5, 6, 7, 8}},
        VectorMessage{std::vector<std::uint32_t>{9, 10, 11, 12}},
        VectorMessage{std::vector<std::uint32_t>{13, 14, 15, 16}},
    };
    const std::array<VectorMessage, 4> vector_big{
        VectorMessage{make_u32_vector(1000, 1)},
        VectorMessage{make_u32_vector(1000, 2)},
        VectorMessage{make_u32_vector(1000, 3)},
        VectorMessage{make_u32_vector(1000, 4)},
    };
    const std::array<ArrayMessage<4>, 4> array_small{
        ArrayMessage<4>{make_u32_array<4>(1)},
        ArrayMessage<4>{make_u32_array<4>(5)},
        ArrayMessage<4>{make_u32_array<4>(9)},
        ArrayMessage<4>{make_u32_array<4>(13)},
    };
    const std::array<ArrayMessage<1000>, 4> array_big{
        ArrayMessage<1000>{make_u32_array<1000>(1)},
        ArrayMessage<1000>{make_u32_array<1000>(2)},
        ArrayMessage<1000>{make_u32_array<1000>(3)},
        ArrayMessage<1000>{make_u32_array<1000>(4)},
    };
    const std::array<TupleMessage, 4> tuple{
        TupleMessage{std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>{7, "tuple-a", {1, 2, 3}}},
        TupleMessage{std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>{11, "tuple-b", {4, 5, 6}}},
        TupleMessage{std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>{13, "tuple-c", {7, 8, 9}}},
        TupleMessage{std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>{17, "tuple-d", {10, 11, 12}}},
    };
    const std::array<VariantMessage, 4> variant{
        VariantMessage{std::uint32_t{17}},
        VariantMessage{std::string{"variant-value"}},
        VariantMessage{std::array<std::uint32_t, 3>{4, 5, 6}},
        VariantMessage{std::string{"variant-alt"}},
    };
    const std::array<Bitset10Message, 4> bitset10{
        Bitset10Message{make_bitset_pattern<10>(true)},
        Bitset10Message{make_bitset_pattern<10>(false)},
        Bitset10Message{make_bitset_pattern<10>(true)},
        Bitset10Message{make_bitset_pattern<10>(false)},
    };
    const std::array<Bitset100Message, 4> bitset100{
        Bitset100Message{make_bitset_pattern<100>(true)},
        Bitset100Message{make_bitset_pattern<100>(false)},
        Bitset100Message{make_bitset_pattern<100>(true)},
        Bitset100Message{make_bitset_pattern<100>(false)},
    };
    const std::array<MapMessage, 4> map_small{
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(2, 1, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(2, 100, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(2, 200, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(2, 300, 8)},
    };
    const std::array<MapMessage, 4> map_big{
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(64, 1, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(64, 1000, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(64, 2000, 8)},
        MapMessage{make_label_map<std::map<std::uint32_t, std::string>>(64, 3000, 8)},
    };
    const std::array<UnorderedMapMessage, 4> unordered_small{
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(2, 1, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(2, 100, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(2, 200, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(2, 300, 8)},
    };
    const std::array<UnorderedMapMessage, 4> unordered_big{
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(64, 1, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(64, 1000, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(64, 2000, 8)},
        UnorderedMapMessage{make_label_map<std::unordered_map<std::uint32_t, std::string>>(64, 3000, 8)},
    };

    const std::array<WideMessage, 4> wide{
        WideMessage{
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        },
        WideMessage{
            101, 102, 103, 104, 105, 106, 107, 108,
            109, 110, 111, 112, 113, 114, 115, 116,
        },
        WideMessage{
            201, 202, 203, 204, 205, 206, 207, 208,
            209, 210, 211, 212, 213, 214, 215, 216,
        },
        WideMessage{
            301, 302, 303, 304, 305, 306, 307, 308,
            309, 310, 311, 312, 313, 314, 315, 316,
        },
    };

    std::vector<std::uint32_t> raw_u32;
    raw_u32.reserve(100);
    for (std::uint32_t value = 1; value <= 100; ++value) {
        raw_u32.push_back(value);
    }
    std::vector<std::uint32_t> raw_u32_1000;
    raw_u32_1000.reserve(1000);
    for (std::uint32_t value = 1; value <= 1000; ++value) {
        raw_u32_1000.push_back(value);
    }

    std::vector<row_entry> entries;
    entries.reserve(27);
    entries.push_back({row_name("bool", 1), [&] { return benchmark_row("bool", boolean); }});
    entries.push_back({row_name("double", 1), [&] { return benchmark_row("double", floating); }});
    entries.push_back({row_name("i64", 1), [&] { return benchmark_row("i64", signed_int); }});
    entries.push_back({row_name("nested", 1), [&] { return benchmark_row("nested", nested); }});
    entries.push_back({row_name("deep_nested", 1), [&] { return benchmark_row("deep_nested", deep_nested); }});
    entries.push_back({row_name("numeric", 1), [&] { return benchmark_row("numeric", numeric); }});
    entries.push_back({row_name("one", 1), [&] { return benchmark_row("one", one_field); }});
    entries.push_back({row_name("small", 1), [&] { return benchmark_row("small", small); }});
    entries.push_back({row_name("string", 1), [&] { return benchmark_row("string", text); }});
    entries.push_back({row_name("tiny", 1), [&] { return benchmark_row("tiny", tiny); }});
    entries.push_back({row_name("u32 raw", 100), [&] { return benchmark_scalar_row("u32 raw[1..100]", raw_u32); }});
    entries.push_back({row_name("u32 raw", 1000), [&] { return benchmark_scalar_row("u32 raw[1..1000]", raw_u32_1000); }});
    entries.push_back({row_name("u64", 1), [&] { return benchmark_row("u64", unsigned_int); }});
    entries.push_back({"optional[4]", [&] { return benchmark_row("optional[4]", optional); }});
    entries.push_back({"vector[4]", [&] { return benchmark_row("vector[4]", vector_small); }});
    entries.push_back({"vector[1000]", [&] { return benchmark_row("vector[1000]", vector_big); }});
    entries.push_back({"array[4]", [&] { return benchmark_row("array[4]", array_small); }});
    entries.push_back({"array[1000]", [&] { return benchmark_row("array[1000]", array_big); }});
    entries.push_back({"tuple[4]", [&] { return benchmark_row("tuple[4]", tuple); }});
    entries.push_back({"variant[4]", [&] { return benchmark_row("variant[4]", variant); }});
    entries.push_back({"bitset[10]", [&] { return benchmark_row("bitset[10]", bitset10); }});
    entries.push_back({"bitset[100]", [&] { return benchmark_row("bitset[100]", bitset100); }});
    entries.push_back({"map[2]", [&] { return benchmark_row("map[2]", map_small); }});
    entries.push_back({"map[64]", [&] { return benchmark_row("map[64]", map_big); }});
    entries.push_back({"unordered_map[2]", [&] { return benchmark_row("unordered_map[2]", unordered_small); }});
    entries.push_back({"unordered_map[64]", [&] { return benchmark_row("unordered_map[64]", unordered_big); }});
    entries.push_back({"wide[16]", [&] { return benchmark_row("wide[16]", wide); }});

    std::sort(entries.begin(), entries.end(), [](const row_entry& a, const row_entry& b) {
        const auto ka = parse_row_sort_key(a.name);
        const auto kb = parse_row_sort_key(b.name);

        if (ka.base != kb.base) {
            return ka.base < kb.base;
        }
        if (ka.size != kb.size) {
            return ka.size < kb.size;
        }
        return a.name < b.name;
    });

    std::cout << "iterations: " << iterations
              << " repeats: " << repeats
              << " metric: ns/op\n\n";

    if (cli.list_rows) {
        print_row_list(entries, std::cout);
        return 0;
    }

    std::cout << std::left
              << std::setw(18) << "scenario"
              << std::right
              << std::setw(14) << "proto sz"
              << std::setw(14) << "compact sz"
              << std::setw(14) << "proto pack"
              << std::setw(14) << "compact pk"
              << std::setw(10) << "c/p"
              << std::setw(14) << "proto un"
              << std::setw(14) << "compact un"
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
