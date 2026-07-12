#include "benchmark_base.hpp"

#include <contract/contract.hpp>
#include <contract/adapters/binary/all.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

namespace {

#if defined(_MSC_VER)
#    define CONTRACT_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#    define CONTRACT_NOINLINE __attribute__((noinline))
#else
#    define CONTRACT_NOINLINE
#endif

int iterations = 5'000;
constexpr int repeats = 101;

volatile std::uint64_t buffer_version_sink = 0;

enum column : std::size_t {
    out_obj,
    field_get,
    field_ref,
    in_obj_zero_copy,
    in_obj_copy,
    field_set,
    column_count
};

using ratio_value = benchmark_base::quantiles;
using measurement_value = benchmark_base::measurement;
using benchmark_base::consume;

struct BenchStringViewPlain;
struct BenchStringViewHooked;

template<class Plain>
struct supports_copy_read : std::true_type {};

template<>
struct supports_copy_read<BenchStringViewPlain> : std::false_type {};

template<>
struct supports_copy_read<BenchStringViewHooked> : std::false_type {};

template<class Plain>
inline constexpr bool supports_copy_read_v = supports_copy_read<Plain>::value;

using row_values = std::array<std::optional<measurement_value>, column_count>;

std::uint64_t hash_bytes(const unsigned char* data, std::size_t size) {
    std::uint64_t hash = 0;
    for (std::size_t i = 0; i < size; ++i) {
        hash = (hash << 5) | (hash >> 59);
        hash ^= static_cast<std::uint64_t>(data[i]);
    }
    hash ^= static_cast<std::uint64_t>(size) << 56u;
    return hash;
}

template<class... Objects>
void finalize_row_state(
    const unsigned char* data,
    std::size_t size,
    volatile std::uint64_t& sink,
    const Objects&... objects)
{
    buffer_version_sink = (buffer_version_sink << 1) ^ hash_bytes(data, size);
    (benchmark_base::consume_object(sink, objects), ...);
}

using result_row = benchmark_base::table_row<column_count, measurement_value>;

struct suite_config {
    std::size_t text_size;
    std::size_t vector_size;
    std::size_t map_size;
    std::size_t unordered_map_size;
};

std::string row_name(std::string_view base, std::string_view shape) {
    std::string name;
    name.reserve(base.size() + shape.size() + 2);
    name.append(base);
    name.push_back('[');
    name.append(shape);
    name.push_back(']');
    return name;
}

std::string row_name(std::string_view base, std::string_view shape, std::string_view suite) {
    std::string name;
    name.reserve(base.size() + shape.size() + suite.size() + 4);
    name.append(base);
    name.push_back('[');
    name.append(shape);
    name.append(", ");
    name.append(suite);
    name.push_back(']');
    return name;
}

std::string row_name(std::string_view base, std::size_t size) {
    return row_name(base, std::to_string(size));
}

std::string row_name(std::string_view base, std::size_t size, std::string_view suite) {
    return row_name(base, std::to_string(size), suite);
}

template<std::size_t N>
std::array<char, N> make_char_payload(char start) {
    std::array<char, N> value{};
    for (std::size_t i = 0; i < N; ++i) {
        value[i] = static_cast<char>(start + static_cast<char>(i % 26));
    }
    return value;
}

struct BenchNumericPlain {
    std::uint64_t id = 100;
    std::uint32_t count = 42;
    double ratio = 3.25;
    std::uint64_t total = 9000;

    CONTRACT(BenchNumericPlain, (id, 1), (count, 2), (ratio, 3), (total, 4))
};

struct BenchNumericHooked {
    std::uint64_t id = 100;
    std::uint32_t count = 42;
    double ratio = 3.25;
    std::uint64_t total = 9000;

    CONTRACT(BenchNumericHooked, (id, 1), (count, 2), (ratio, 3), (total, 4))
    int contract_get(const contract_fields::id&) const {
        return id;
    }

    template<class Value>
    void contract_set(const contract_fields::id&, Value&& value) {
        id = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    std::uint32_t contract_get(const contract_fields::count&) const {
        return count;
    }

    template<class Value>
    void contract_set(const contract_fields::count&, Value&& value) {
        count = static_cast<std::uint32_t>(std::forward<Value>(value));
    }

    double contract_get(const contract_fields::ratio&) const {
        return ratio;
    }

    template<class Value>
    void contract_set(const contract_fields::ratio&, Value&& value) {
        ratio = static_cast<double>(std::forward<Value>(value));
    }

    std::uint64_t contract_get(const contract_fields::total&) const {
        return total;
    }

    template<class Value>
    void contract_set(const contract_fields::total&, Value&& value) {
        total = static_cast<std::uint64_t>(std::forward<Value>(value));
    }
};

struct BenchStringPlain {
    std::string name = "contract";

    CONTRACT(BenchStringPlain, (name, 1))
};

struct BenchStringHooked {
    std::string name = "contract";

    CONTRACT(BenchStringHooked, (name, 1))

    template<class Value>
    void contract_set(const contract_fields::name&, Value&& value) {
        name = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::name&) const {
        return (name);
    }
};

struct BenchStringViewPlain {
    std::string_view name = "contract";

    CONTRACT(BenchStringViewPlain, (name, 1))
};

struct BenchStringViewHooked {
    std::string_view name = "contract";

    CONTRACT(BenchStringViewHooked, (name, 1))

    template<class Value>
    void contract_set(const contract_fields::name&, Value&& value) {
        name = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::name&) const {
        return name;
    }
};

struct BenchCStringPlain {
    const char* category = "adapter";

    CONTRACT(BenchCStringPlain, (category, 1))
};

struct BenchCStringHooked {
    const char* category = "adapter";

    CONTRACT(BenchCStringHooked, (category, 1))

    template<class Value>
    void contract_set(const contract_fields::category&, Value&& value) {
        category = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::category&) const {
        return category;
    }
};

#define DEFINE_BENCH_CHAR_ARRAY_VARIANT(SUFFIX, N) \
struct BenchCharArrayPlain##SUFFIX { \
    char tag[N] = {}; \
 \
    CONTRACT(BenchCharArrayPlain##SUFFIX, (tag, 1)) \
};

#define DEFINE_BENCH_CHAR_ARRAY_HOOKED_VARIANT(SUFFIX, N) \
struct BenchCharArrayHooked##SUFFIX { \
    char tag[N] = {}; \
 \
    CONTRACT(BenchCharArrayHooked##SUFFIX, (tag, 1)) \
 \
    template<class Value> \
    void contract_set(const contract_fields::tag&, Value&& value) { \
        std::memcpy(tag, std::forward<Value>(value), sizeof(tag)); \
    } \
 \
    decltype(auto) contract_get(const contract_fields::tag&) const { \
        return (tag); \
    } \
};

DEFINE_BENCH_CHAR_ARRAY_VARIANT(8, 8)
DEFINE_BENCH_CHAR_ARRAY_VARIANT(1000, 1000)
DEFINE_BENCH_CHAR_ARRAY_HOOKED_VARIANT(8, 8)
DEFINE_BENCH_CHAR_ARRAY_HOOKED_VARIANT(1000, 1000)

#undef DEFINE_BENCH_CHAR_ARRAY_VARIANT
#undef DEFINE_BENCH_CHAR_ARRAY_HOOKED_VARIANT

struct BenchRequestBase {
    std::uint64_t timestamp = 7;
    std::uint64_t service = 11;

    CONTRACT(BenchRequestBase, (timestamp, 1), (service, 2))
};

struct BenchRequestEventPlain : BenchRequestBase {
    std::uint64_t user_id = 42;

    CONTRACT(BenchRequestEventPlain, BASE(BenchRequestBase, 1000), (user_id, 1101))
};

struct BenchRequestEventHooked : BenchRequestBase {
    std::uint64_t user_id = 42;

    CONTRACT(BenchRequestEventHooked, BASE(BenchRequestBase, 1000), (user_id, 1101))

    template<class Value>
    void contract_set(const BenchRequestBase::contract_fields::timestamp&, Value&& value) {
        timestamp = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    template<class Value>
    void contract_set(const BenchRequestBase::contract_fields::service&, Value&& value) {
        service = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    template<class Value>
    void contract_set(const contract_fields::user_id&, Value&& value) {
        user_id = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    decltype(auto) contract_get(const BenchRequestBase::contract_fields::timestamp&) const {
        return timestamp;
    }

    decltype(auto) contract_get(const BenchRequestBase::contract_fields::service&) const {
        return service;
    }

    decltype(auto) contract_get(const contract_fields::user_id&) const {
        return user_id;
    }
};

struct BenchTraceContext {
    std::uint64_t trace_id = 5;

    CONTRACT(BenchTraceContext, (trace_id, 1))
};

struct BenchRoutedEventPlain : BenchRequestBase, BenchTraceContext {
    std::uint64_t route_id = 9;

    CONTRACT(BenchRoutedEventPlain,
        BASE(BenchRequestBase, 1000),
        BASE(BenchTraceContext, 2000),
        (route_id, 3001))
};

struct BenchRoutedEventHooked : BenchRequestBase, BenchTraceContext {
    std::uint64_t route_id = 9;

    CONTRACT(BenchRoutedEventHooked,
        BASE(BenchRequestBase, 1000),
        BASE(BenchTraceContext, 2000),
        (route_id, 3001))

    template<class Value>
    void contract_set(const BenchRequestBase::contract_fields::timestamp&, Value&& value) {
        timestamp = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    template<class Value>
    void contract_set(const BenchRequestBase::contract_fields::service&, Value&& value) {
        service = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    template<class Value>
    void contract_set(const BenchTraceContext::contract_fields::trace_id&, Value&& value) {
        trace_id = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    template<class Value>
    void contract_set(const contract_fields::route_id&, Value&& value) {
        route_id = static_cast<std::uint64_t>(std::forward<Value>(value));
    }

    decltype(auto) contract_get(const BenchRequestBase::contract_fields::timestamp&) const {
        return timestamp;
    }

    decltype(auto) contract_get(const BenchRequestBase::contract_fields::service&) const {
        return service;
    }

    decltype(auto) contract_get(const BenchTraceContext::contract_fields::trace_id&) const {
        return trace_id;
    }

    decltype(auto) contract_get(const contract_fields::route_id&) const {
        return route_id;
    }
};

struct BenchVectorPlain {
    std::vector<std::uint32_t> values{1, 2, 3, 4};

    CONTRACT(BenchVectorPlain, (values, 1))
};

struct BenchVectorHooked {
    std::vector<std::uint32_t> values{1, 2, 3, 4};

    CONTRACT(BenchVectorHooked, (values, 1))

    template<class Value>
    void contract_set(const contract_fields::values&, Value&& value) {
        values = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::values&) const {
        return (values);
    }
};

std::vector<std::uint32_t> make_u32_sequence(std::size_t count, std::uint32_t start) {
    std::vector<std::uint32_t> values;
    values.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        values.push_back(start + static_cast<std::uint32_t>(i));
    }
    return values;
}

#define DEFINE_BENCH_ARRAY_VARIANT(SUFFIX, N) \
struct BenchArrayPlain##SUFFIX { \
    std::array<std::uint32_t, N> values{}; \
 \
    CONTRACT(BenchArrayPlain##SUFFIX, (values, 1)) \
}; \
 \
struct BenchArrayHooked##SUFFIX { \
    std::array<std::uint32_t, N> values{}; \
 \
    CONTRACT(BenchArrayHooked##SUFFIX, (values, 1)) \
 \
    template<class Value> \
    void contract_set(const contract_fields::values&, Value&& value) { \
        values = std::forward<Value>(value); \
    } \
 \
    decltype(auto) contract_get(const contract_fields::values&) const { \
        return (values); \
    } \
}; \
 \
CONTRACT_NOINLINE void manual_write(const BenchArrayPlain##SUFFIX& record, unsigned char* data) { \
    std::memcpy(data, record.values.data(), sizeof(record.values)); \
} \
 \
CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchArrayPlain##SUFFIX& record) { \
    std::memcpy(record.values.data(), data, sizeof(record.values)); \
}; \

DEFINE_BENCH_ARRAY_VARIANT(4, 4)
DEFINE_BENCH_ARRAY_VARIANT(1000, 1000)

#undef DEFINE_BENCH_ARRAY_VARIANT

CONTRACT_NOINLINE void manual_write(const BenchArrayHooked4& record, unsigned char* data) {
    std::memcpy(data, record.values.data(), sizeof(record.values));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchArrayHooked4& record) {
    std::memcpy(record.values.data(), data, sizeof(record.values));
}

CONTRACT_NOINLINE void manual_write(const BenchArrayHooked1000& record, unsigned char* data) {
    std::memcpy(data, record.values.data(), sizeof(record.values));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchArrayHooked1000& record) {
    std::memcpy(record.values.data(), data, sizeof(record.values));
}

template<std::size_t N>
std::array<std::uint32_t, N> make_u32_array(std::uint32_t start) {
    std::array<std::uint32_t, N> values{};
    for (std::size_t i = 0; i < N; ++i) {
        values[i] = start + static_cast<std::uint32_t>(i);
    }
    return values;
}

struct BenchTuplePlain {
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        7,
        "tuple",
        {1, 2, 3}
    };

    CONTRACT(BenchTuplePlain, (payload, 1))
};

struct BenchTupleHooked {
    std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        7,
        "tuple",
        {1, 2, 3}
    };

    CONTRACT(BenchTupleHooked, (payload, 1))

    template<class Value>
    void contract_set(const contract_fields::payload&, Value&& value) {
        payload = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::payload&) const {
        return (payload);
    }
};

struct BenchVariantPlain {
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        std::string("variant-value")
    };

    CONTRACT(BenchVariantPlain, (payload, 1))
};

struct BenchVariantHooked {
    std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 3>> payload{
        std::string("variant-value")
    };

    CONTRACT(BenchVariantHooked, (payload, 1))

    template<class Value>
    void contract_set(const contract_fields::payload&, Value&& value) {
        payload = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::payload&) const {
        return (payload);
    }
};

struct BenchMapPlain {
    std::map<std::uint32_t, std::string> labels{{1, "one"}, {3, "three"}};

    CONTRACT(BenchMapPlain, (labels, 1))
};

struct BenchMapHooked {
    std::map<std::uint32_t, std::string> labels{{1, "one"}, {3, "three"}};

    CONTRACT(BenchMapHooked, (labels, 1))

    template<class Value>
    void contract_set(const contract_fields::labels&, Value&& value) {
        labels = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::labels&) const {
        return (labels);
    }
};

struct BenchOptionalPlain {
    std::optional<std::uint32_t> count{42};

    CONTRACT(BenchOptionalPlain, (count, 1))
};

struct BenchOptionalHooked {
    std::optional<std::uint32_t> count{42};

    CONTRACT(BenchOptionalHooked, (count, 1))

    template<class Value>
    void contract_set(const contract_fields::count&, Value&& value) {
        count = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::count&) const {
        return (count);
    }
};

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

#define DEFINE_BENCH_BITSET_VARIANT(SUFFIX, N) \
struct BenchBitsetPlain##SUFFIX { \
    std::bitset<N> flags{}; \
 \
    CONTRACT(BenchBitsetPlain##SUFFIX, (flags, 1)) \
}; \
 \
struct BenchBitsetHooked##SUFFIX { \
    std::bitset<N> flags{}; \
 \
    CONTRACT(BenchBitsetHooked##SUFFIX, (flags, 1)) \
 \
    template<class Value> \
    void contract_set(const contract_fields::flags&, Value&& value) { \
        flags = std::forward<Value>(value); \
    } \
 \
    decltype(auto) contract_get(const contract_fields::flags&) const { \
        return (flags); \
    } \
}; \

DEFINE_BENCH_BITSET_VARIANT(10, 10)
DEFINE_BENCH_BITSET_VARIANT(100, 100)

#undef DEFINE_BENCH_BITSET_VARIANT

template<std::size_t N>
CONTRACT_NOINLINE void manual_write(const std::bitset<N>& value, unsigned char* data) {
    if constexpr (N <= 64) {
        if constexpr (N != 0) {
            const std::uint64_t raw = static_cast<std::uint64_t>(value.to_ullong());
            std::memcpy(data, &raw, sizeof(raw));
        }
    } else {
        const std::size_t byte_count = (N + 7) / 8;
        for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
            unsigned char byte = 0;
            for (std::size_t bit = 0; bit < 8; ++bit) {
                const std::size_t bit_index = byte_index * 8 + bit;
                if (bit_index < N && value.test(bit_index)) {
                    byte |= static_cast<unsigned char>(1u << bit);
                }
            }
            std::memcpy(data + byte_index, &byte, 1);
        }
    }
}

template<std::size_t N>
CONTRACT_NOINLINE void manual_read(const unsigned char* data, std::bitset<N>& value) {
    value.reset();

    if constexpr (N <= 64) {
        if constexpr (N != 0) {
            std::uint64_t raw = 0;
            std::memcpy(&raw, data, sizeof(raw));
            value = std::bitset<N>(raw);
        }
    } else {
        const std::size_t byte_count = (N + 7) / 8;
        for (std::size_t byte_index = 0; byte_index < byte_count; ++byte_index) {
            unsigned char byte = 0;
            std::memcpy(&byte, data + byte_index, 1);
            for (std::size_t bit = 0; bit < 8; ++bit) {
                const std::size_t bit_index = byte_index * 8 + bit;
                if (bit_index < N && ((byte >> bit) & 0x1u)) {
                    value.set(bit_index);
                }
            }
        }
    }
}

CONTRACT_NOINLINE void manual_write(const BenchBitsetPlain10& record, unsigned char* data) {
    manual_write(record.flags, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchBitsetPlain10& record) {
    manual_read(data, record.flags);
}

CONTRACT_NOINLINE void manual_write(const BenchBitsetPlain100& record, unsigned char* data) {
    manual_write(record.flags, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchBitsetPlain100& record) {
    manual_read(data, record.flags);
}

CONTRACT_NOINLINE void manual_write(const BenchBitsetHooked10& record, unsigned char* data) {
    manual_write(record.flags, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchBitsetHooked10& record) {
    manual_read(data, record.flags);
}

CONTRACT_NOINLINE void manual_write(const BenchBitsetHooked100& record, unsigned char* data) {
    manual_write(record.flags, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchBitsetHooked100& record) {
    manual_read(data, record.flags);
}

void write_size(unsigned char*& out, std::size_t value);
std::size_t read_size(const unsigned char*& in);

using tuple_payload_t = std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>;

tuple_payload_t make_tuple_payload(std::size_t slot) {
    switch (slot % 5) {
    case 0:
        return tuple_payload_t{
            std::uint32_t{17},
            std::string("tuple-a"),
            std::array<std::uint32_t, 3>{4, 5, 6}
        };
    case 1:
        return tuple_payload_t{
            std::uint32_t{19},
            std::string("tuple-b"),
            std::array<std::uint32_t, 3>{7, 8, 9}
        };
    case 2:
        return tuple_payload_t{
            std::uint32_t{21},
            std::string("tuple-c"),
            std::array<std::uint32_t, 3>{10, 11, 12}
        };
    case 3:
        return tuple_payload_t{
            std::uint32_t{23},
            std::string("tuple-d"),
            std::array<std::uint32_t, 3>{13, 14, 15}
        };
    default:
        return tuple_payload_t{
            std::uint32_t{25},
            std::string("tuple-e"),
            std::array<std::uint32_t, 3>{16, 17, 18}
        };
    }
}

using variant_payload_t = std::variant<std::uint32_t, std::string, std::array<std::uint32_t, 3>>;

variant_payload_t make_variant_payload(std::size_t index) {
    switch (index % 5) {
    case 0:
        return std::uint32_t{17};
    case 1:
        return std::string("variant-value");
    case 2:
        return std::array<std::uint32_t, 3>{4, 5, 6};
    case 3:
        return std::uint32_t{29};
    default:
        return std::string("variant-alt");
    }
}

CONTRACT_NOINLINE void manual_write(const BenchTuplePlain& record, unsigned char* data) {
    std::apply([&](const auto& first, const auto& text, const auto& tail) {
        std::memcpy(data, &first, sizeof(first));
        data += sizeof(first);

        write_size(data, text.size());
        std::memcpy(data, text.data(), text.size());
        data += text.size();

        std::memcpy(data, tail.data(), sizeof(tail));
    }, record.payload);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchTuplePlain& record) {
    std::uint32_t first = 0;
    std::memcpy(&first, data, sizeof(first));
    data += sizeof(first);

    const std::size_t size = read_size(data);
    std::string text;
    text.resize(size);
    if (size != 0) {
        std::memcpy(text.data(), data, size);
        data += size;
    }

    std::array<std::uint32_t, 3> tail{};
    std::memcpy(tail.data(), data, sizeof(tail));

    record.payload = tuple_payload_t{
        first,
        std::move(text),
        tail
    };
}

CONTRACT_NOINLINE void manual_write(const BenchTupleHooked& record, unsigned char* data) {
    std::apply([&](const auto& first, const auto& text, const auto& tail) {
        std::memcpy(data, &first, sizeof(first));
        data += sizeof(first);

        write_size(data, text.size());
        std::memcpy(data, text.data(), text.size());
        data += text.size();

        std::memcpy(data, tail.data(), sizeof(tail));
    }, record.payload);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchTupleHooked& record) {
    std::uint32_t first = 0;
    std::memcpy(&first, data, sizeof(first));
    data += sizeof(first);

    const std::size_t size = read_size(data);
    std::string text;
    text.resize(size);
    if (size != 0) {
        std::memcpy(text.data(), data, size);
        data += size;
    }

    std::array<std::uint32_t, 3> tail{};
    std::memcpy(tail.data(), data, sizeof(tail));

    record.payload = tuple_payload_t{
        first,
        std::move(text),
        tail
    };
}

CONTRACT_NOINLINE void manual_write(const BenchVariantPlain& record, unsigned char* data) {
    const std::size_t index = record.payload.index();
    write_size(data, index);

    std::visit([&](const auto& item) {
        using item_type = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<item_type, std::uint32_t>) {
            std::memcpy(data, &item, sizeof(item));
            data += sizeof(item);
        } else if constexpr (std::is_same_v<item_type, std::string>) {
            write_size(data, item.size());
            std::memcpy(data, item.data(), item.size());
            data += item.size();
        } else {
            std::memcpy(data, item.data(), sizeof(item));
            data += sizeof(item);
        }
    }, record.payload);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchVariantPlain& record) {
    const std::size_t index = read_size(data);

    switch (index) {
    case 0: {
        std::uint32_t value = 0;
        std::memcpy(&value, data, sizeof(value));
        data += sizeof(value);
        record.payload = value;
        break;
    }
    case 1: {
        const std::size_t size = read_size(data);
        std::string value;
        value.resize(size);
        if (size != 0) {
            std::memcpy(value.data(), data, size);
            data += size;
        }
        record.payload = std::move(value);
        break;
    }
    case 2: {
        std::array<std::uint32_t, 3> value{};
        std::memcpy(value.data(), data, sizeof(value));
        data += sizeof(value);
        record.payload = value;
        break;
    }
    default:
        throw std::out_of_range("manual variant read: variant index out of range");
    }
}

CONTRACT_NOINLINE void manual_write(const BenchVariantHooked& record, unsigned char* data) {
    const std::size_t index = record.payload.index();
    write_size(data, index);

    std::visit([&](const auto& item) {
        using item_type = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<item_type, std::uint32_t>) {
            std::memcpy(data, &item, sizeof(item));
            data += sizeof(item);
        } else if constexpr (std::is_same_v<item_type, std::string>) {
            write_size(data, item.size());
            std::memcpy(data, item.data(), item.size());
            data += item.size();
        } else {
            std::memcpy(data, item.data(), sizeof(item));
            data += sizeof(item);
        }
    }, record.payload);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchVariantHooked& record) {
    const std::size_t index = read_size(data);

    switch (index) {
    case 0: {
        std::uint32_t value = 0;
        std::memcpy(&value, data, sizeof(value));
        data += sizeof(value);
        record.payload = value;
        break;
    }
    case 1: {
        const std::size_t size = read_size(data);
        std::string value;
        value.resize(size);
        if (size != 0) {
            std::memcpy(value.data(), data, size);
            data += size;
        }
        record.payload = std::move(value);
        break;
    }
    case 2: {
        std::array<std::uint32_t, 3> value{};
        std::memcpy(value.data(), data, sizeof(value));
        data += sizeof(value);
        record.payload = value;
        break;
    }
    default:
        throw std::out_of_range("manual variant read: variant index out of range");
    }
}

struct BenchUnorderedMapPlain {
    std::unordered_map<std::uint32_t, std::string> labels{{3, "three"}, {1, "one"}};

    CONTRACT(BenchUnorderedMapPlain, (labels, 1))
};

struct BenchUnorderedMapHooked {
    std::unordered_map<std::uint32_t, std::string> labels{{3, "three"}, {1, "one"}};

    CONTRACT(BenchUnorderedMapHooked, (labels, 1))

    template<class Value>
    void contract_set(const contract_fields::labels&, Value&& value) {
        labels = std::forward<Value>(value);
    }

    decltype(auto) contract_get(const contract_fields::labels&) const {
        return (labels);
    }
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

CONTRACT_NOINLINE void exercise_hooked_fixtures(volatile std::uint64_t& sink) {
    BenchNumericHooked numeric;
    numeric.contract_set(BenchNumericHooked::contract_fields::id{"id", {}}, 101u);
    numeric.contract_set(BenchNumericHooked::contract_fields::count{"count", {}}, 43u);
    numeric.contract_set(BenchNumericHooked::contract_fields::ratio{"ratio", {}}, 4.25);
    numeric.contract_set(BenchNumericHooked::contract_fields::total{"total", {}}, 9001u);
    sink += numeric.contract_get(BenchNumericHooked::contract_fields::id{"id", {}});
    sink += numeric.contract_get(BenchNumericHooked::contract_fields::count{"count", {}});
    sink += static_cast<std::uint64_t>(numeric.contract_get(BenchNumericHooked::contract_fields::ratio{"ratio", {}}));
    sink += numeric.contract_get(BenchNumericHooked::contract_fields::total{"total", {}});

    BenchStringHooked string_hooked;
    string_hooked.contract_set(BenchStringHooked::contract_fields::name{"name", {}}, std::string("hooked"));
    sink += string_hooked.contract_get(BenchStringHooked::contract_fields::name{"name", {}}).size();

    std::string_view view_storage = "hooked-view";
    BenchStringViewHooked view_hooked;
    view_hooked.contract_set(BenchStringViewHooked::contract_fields::name{"name", {}}, view_storage);
    sink += view_hooked.contract_get(BenchStringViewHooked::contract_fields::name{"name", {}}).size();

    const char* cstring_storage = "hooked-cstr";
    BenchCStringHooked cstring_hooked;
    cstring_hooked.contract_set(BenchCStringHooked::contract_fields::category{"category", {}}, cstring_storage);
    sink += std::char_traits<char>::length(cstring_hooked.contract_get(BenchCStringHooked::contract_fields::category{"category", {}}));

    BenchCharArrayHooked8 char8;
    char8.contract_set(BenchCharArrayHooked8::contract_fields::tag{"tag", {}}, make_char_payload<8>('h').data());
    sink += static_cast<unsigned char>(char8.contract_get(BenchCharArrayHooked8::contract_fields::tag{"tag", {}})[0]);

    BenchCharArrayHooked1000 char1000;
    char1000.contract_set(BenchCharArrayHooked1000::contract_fields::tag{"tag", {}}, make_char_payload<1000>('h').data());
    sink += static_cast<unsigned char>(char1000.contract_get(BenchCharArrayHooked1000::contract_fields::tag{"tag", {}})[0]);

    BenchRequestEventHooked request;
    request.contract_set(BenchRequestBase::contract_fields::timestamp{"timestamp", {}}, 8u);
    request.contract_set(BenchRequestBase::contract_fields::service{"service", {}}, 12u);
    request.contract_set(BenchRequestEventHooked::contract_fields::user_id{"user_id", {}}, 43u);
    sink += request.contract_get(BenchRequestBase::contract_fields::timestamp{"timestamp", {}});
    sink += request.contract_get(BenchRequestBase::contract_fields::service{"service", {}});
    sink += request.contract_get(BenchRequestEventHooked::contract_fields::user_id{"user_id", {}});

    BenchRoutedEventHooked routed;
    routed.contract_set(BenchRequestBase::contract_fields::timestamp{"timestamp", {}}, 8u);
    routed.contract_set(BenchRequestBase::contract_fields::service{"service", {}}, 12u);
    routed.contract_set(BenchTraceContext::contract_fields::trace_id{"trace_id", {}}, 6u);
    routed.contract_set(BenchRoutedEventHooked::contract_fields::route_id{"route_id", {}}, 10u);
    sink += routed.contract_get(BenchRequestBase::contract_fields::timestamp{"timestamp", {}});
    sink += routed.contract_get(BenchRequestBase::contract_fields::service{"service", {}});
    sink += routed.contract_get(BenchTraceContext::contract_fields::trace_id{"trace_id", {}});
    sink += routed.contract_get(BenchRoutedEventHooked::contract_fields::route_id{"route_id", {}});

    BenchVectorHooked vector;
    vector.contract_set(BenchVectorHooked::contract_fields::values{"values", {}}, std::vector<std::uint32_t>{7, 8, 9, 10});
    sink += vector.contract_get(BenchVectorHooked::contract_fields::values{"values", {}})[0];

    BenchArrayHooked4 array4;
    array4.contract_set(BenchArrayHooked4::contract_fields::values{"values", {}}, std::array<std::uint32_t, 4>{9, 10, 11, 12});
    sink += array4.contract_get(BenchArrayHooked4::contract_fields::values{"values", {}})[0];

    BenchTupleHooked tuple;
    tuple.contract_set(BenchTupleHooked::contract_fields::payload{"payload", {}}, std::tuple<std::uint32_t, std::string, std::array<std::uint32_t, 3>>{
        19,
        "tuple-alt",
        {7, 8, 9}
    });
    sink += std::get<0>(tuple.contract_get(BenchTupleHooked::contract_fields::payload{"payload", {}}));

    BenchVariantHooked variant;
    variant.contract_set(BenchVariantHooked::contract_fields::payload{"payload", {}}, std::array<std::uint32_t, 3>{4, 5, 6});
    sink += std::visit([](const auto& item) -> std::uint64_t {
        using item_type = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<item_type, std::uint32_t>) {
            return item;
        } else if constexpr (std::is_same_v<item_type, std::string>) {
            return item.size();
        } else {
            return item[0];
        }
    }, variant.contract_get(BenchVariantHooked::contract_fields::payload{"payload", {}}));

    BenchMapHooked map;
    map.contract_set(BenchMapHooked::contract_fields::labels{"labels", {}}, make_label_map<std::map<std::uint32_t, std::string>>(2, 9, 8));
    sink += map.contract_get(BenchMapHooked::contract_fields::labels{"labels", {}}).size();

    BenchOptionalHooked optional;
    optional.contract_set(BenchOptionalHooked::contract_fields::count{"count", {}}, std::optional<std::uint32_t>{44});
    sink += optional.contract_get(BenchOptionalHooked::contract_fields::count{"count", {}}).value_or(0);

    BenchBitsetHooked10 bitset10;
    bitset10.contract_set(BenchBitsetHooked10::contract_fields::flags{"flags", {}}, std::bitset<10>{0x155});
    sink += bitset10.contract_get(BenchBitsetHooked10::contract_fields::flags{"flags", {}}).count();

    BenchBitsetHooked100 bitset100;
    bitset100.contract_set(BenchBitsetHooked100::contract_fields::flags{"flags", {}}, std::bitset<100>{0x5555});
    sink += bitset100.contract_get(BenchBitsetHooked100::contract_fields::flags{"flags", {}}).count();

    BenchUnorderedMapHooked unordered;
    unordered.contract_set(BenchUnorderedMapHooked::contract_fields::labels{"labels", {}}, make_label_map<std::unordered_map<std::uint32_t, std::string>>(2, 17, 8));
    sink += unordered.contract_get(BenchUnorderedMapHooked::contract_fields::labels{"labels", {}}).size();
}

void write_size(unsigned char*& out, std::size_t value) {
    std::memcpy(out, &value, sizeof(value));
    out += sizeof(value);
}

std::size_t read_size(const unsigned char*& in) {
    std::size_t value = 0;
    std::memcpy(&value, in, sizeof(value));
    in += sizeof(value);
    return value;
}

struct no_read_view_input {
    explicit no_read_view_input(const unsigned char* data, std::size_t size)
        : in(data) {
        (void)size;
    }

    std::size_t read(void* out, std::size_t size) {
        return in.read(out, size);
    }

    const unsigned char* current() const {
        return in.current();
    }

private:
    contract::io::input in;
};

template<class Object>
CONTRACT_NOINLINE void write_contract(const Object& obj, unsigned char* data, std::size_t size) {
    contract::adapters::binary::writer<> out(data);
    out << obj;
    (void)size;
}

template<class Object>
CONTRACT_NOINLINE void read_contract_view(const unsigned char* data, std::size_t size, Object& obj) {
    contract::adapters::binary::reader<> in(data);
    in >> obj;
    (void)size;
}

template<class Object>
CONTRACT_NOINLINE void read_contract_copy(const unsigned char* data, std::size_t size, Object& obj) {
    contract::adapters::binary::reader<no_read_view_input> in(
        no_read_view_input{data, size});
    in >> obj;
}

CONTRACT_NOINLINE void manual_write(const BenchNumericPlain& record, unsigned char* data) {
    std::memcpy(data, &record.id, sizeof(record.id));
    data += sizeof(record.id);
    std::memcpy(data, &record.count, sizeof(record.count));
    data += sizeof(record.count);
    std::memcpy(data, &record.ratio, sizeof(record.ratio));
    data += sizeof(record.ratio);
    std::memcpy(data, &record.total, sizeof(record.total));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchNumericPlain& record) {
    std::memcpy(&record.id, data, sizeof(record.id));
    data += sizeof(record.id);
    std::memcpy(&record.count, data, sizeof(record.count));
    data += sizeof(record.count);
    std::memcpy(&record.ratio, data, sizeof(record.ratio));
    data += sizeof(record.ratio);
    std::memcpy(&record.total, data, sizeof(record.total));
}

CONTRACT_NOINLINE void manual_write(const BenchNumericHooked& record, unsigned char* data) {
    std::memcpy(data, &record.id, sizeof(record.id));
    data += sizeof(record.id);
    std::memcpy(data, &record.count, sizeof(record.count));
    data += sizeof(record.count);
    std::memcpy(data, &record.ratio, sizeof(record.ratio));
    data += sizeof(record.ratio);
    std::memcpy(data, &record.total, sizeof(record.total));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchNumericHooked& record) {
    std::memcpy(&record.id, data, sizeof(record.id));
    data += sizeof(record.id);
    std::memcpy(&record.count, data, sizeof(record.count));
    data += sizeof(record.count);
    std::memcpy(&record.ratio, data, sizeof(record.ratio));
    data += sizeof(record.ratio);
    std::memcpy(&record.total, data, sizeof(record.total));
}

CONTRACT_NOINLINE void manual_write(const BenchStringPlain& record, unsigned char* data) {
    write_size(data, record.name.size());
    std::memcpy(data, record.name.data(), record.name.size());
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchStringPlain& record) {
    const std::size_t size = read_size(data);
    record.name.resize(size);
    std::memcpy(record.name.data(), data, size);
}

CONTRACT_NOINLINE void manual_write(const BenchStringHooked& record, unsigned char* data) {
    write_size(data, record.name.size());
    std::memcpy(data, record.name.data(), record.name.size());
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchStringHooked& record) {
    const std::size_t size = read_size(data);
    record.name.resize(size);
    std::memcpy(record.name.data(), data, size);
}

CONTRACT_NOINLINE void manual_write(const BenchStringViewPlain& record, unsigned char* data) {
    write_size(data, record.name.size());
    std::memcpy(data, record.name.data(), record.name.size());
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchStringViewPlain& record) {
    const std::size_t size = read_size(data);
    record.name = std::string_view{reinterpret_cast<const char*>(data), size};
}

CONTRACT_NOINLINE void manual_write(const BenchStringViewHooked& record, unsigned char* data) {
    write_size(data, record.name.size());
    std::memcpy(data, record.name.data(), record.name.size());
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchStringViewHooked& record) {
    const std::size_t size = read_size(data);
    record.name = std::string_view{reinterpret_cast<const char*>(data), size};
}

CONTRACT_NOINLINE void manual_write(const BenchCStringPlain& record, unsigned char* data) {
    const std::size_t size = std::char_traits<char>::length(record.category);
    write_size(data, size);
    std::memcpy(data, record.category, size);
    data += size;
    const char zero = '\0';
    std::memcpy(data, &zero, 1);
}

CONTRACT_NOINLINE void manual_write(const BenchCStringHooked& record, unsigned char* data) {
    const std::size_t size = std::char_traits<char>::length(record.category);
    write_size(data, size);
    std::memcpy(data, record.category, size);
    data += size;
    const char zero = '\0';
    std::memcpy(data, &zero, 1);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchCStringHooked& record) {
    const std::size_t size = read_size(data);
    record.category = reinterpret_cast<const char*>(data);
    data += size;
}

template<class Record>
CONTRACT_NOINLINE void manual_write(const Record& record, unsigned char* data) {
    std::memcpy(data, record.tag, sizeof(record.tag));
}

template<class Record>
CONTRACT_NOINLINE void manual_read(const unsigned char* data, Record& record) {
    std::memcpy(record.tag, data, sizeof(record.tag));
}

CONTRACT_NOINLINE void manual_write(const BenchRequestEventPlain& record, unsigned char* data) {
    std::memcpy(data, &record.timestamp, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(data, &record.service, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(data, &record.user_id, sizeof(record.user_id));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchRequestEventPlain& record) {
    std::memcpy(&record.timestamp, data, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(&record.service, data, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(&record.user_id, data, sizeof(record.user_id));
}

CONTRACT_NOINLINE void manual_write(const BenchRequestEventHooked& record, unsigned char* data) {
    std::memcpy(data, &record.timestamp, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(data, &record.service, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(data, &record.user_id, sizeof(record.user_id));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchRequestEventHooked& record) {
    std::memcpy(&record.timestamp, data, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(&record.service, data, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(&record.user_id, data, sizeof(record.user_id));
}

CONTRACT_NOINLINE void manual_write(const BenchRoutedEventPlain& record, unsigned char* data) {
    std::memcpy(data, &record.timestamp, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(data, &record.service, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(data, &record.trace_id, sizeof(record.trace_id));
    data += sizeof(record.trace_id);
    std::memcpy(data, &record.route_id, sizeof(record.route_id));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchRoutedEventPlain& record) {
    std::memcpy(&record.timestamp, data, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(&record.service, data, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(&record.trace_id, data, sizeof(record.trace_id));
    data += sizeof(record.trace_id);
    std::memcpy(&record.route_id, data, sizeof(record.route_id));
}

CONTRACT_NOINLINE void manual_write(const BenchRoutedEventHooked& record, unsigned char* data) {
    std::memcpy(data, &record.timestamp, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(data, &record.service, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(data, &record.trace_id, sizeof(record.trace_id));
    data += sizeof(record.trace_id);
    std::memcpy(data, &record.route_id, sizeof(record.route_id));
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchRoutedEventHooked& record) {
    std::memcpy(&record.timestamp, data, sizeof(record.timestamp));
    data += sizeof(record.timestamp);
    std::memcpy(&record.service, data, sizeof(record.service));
    data += sizeof(record.service);
    std::memcpy(&record.trace_id, data, sizeof(record.trace_id));
    data += sizeof(record.trace_id);
    std::memcpy(&record.route_id, data, sizeof(record.route_id));
}

CONTRACT_NOINLINE void manual_write(const BenchVectorPlain& record, unsigned char* data) {
    write_size(data, record.values.size());
    if (!record.values.empty()) {
        std::memcpy(data, record.values.data(), record.values.size() * sizeof(std::uint32_t));
    }
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchVectorPlain& record) {
    const std::size_t count = read_size(data);
    record.values.resize(count);
    if (count != 0) {
        std::memcpy(record.values.data(), data, count * sizeof(std::uint32_t));
    }
}

CONTRACT_NOINLINE void manual_write(const BenchVectorHooked& record, unsigned char* data) {
    write_size(data, record.values.size());
    if (!record.values.empty()) {
        std::memcpy(data, record.values.data(), record.values.size() * sizeof(std::uint32_t));
    }
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchVectorHooked& record) {
    const std::size_t count = read_size(data);
    record.values.resize(count);
    if (count != 0) {
        std::memcpy(record.values.data(), data, count * sizeof(std::uint32_t));
    }
}

template<class Map>
CONTRACT_NOINLINE void manual_write_map_like(const Map& labels, unsigned char* data) {
    write_size(data, labels.size());
    for (const auto& [key, value] : labels) {
        std::memcpy(data, &key, sizeof(key));
        data += sizeof(key);
        write_size(data, value.size());
        std::memcpy(data, value.data(), value.size());
        data += value.size();
    }
}

template<class Map>
CONTRACT_NOINLINE void manual_read_map_like(const unsigned char* data, Map& labels) {
    const std::size_t count = read_size(data);
    labels.clear();
    if constexpr (has_reserve<Map>::value) {
        labels.reserve(count);
    }
    for (std::size_t i = 0; i < count; ++i) {
        typename Map::key_type key{};
        std::memcpy(&key, data, sizeof(key));
        data += sizeof(key);
        const std::size_t size = read_size(data);
        std::string value;
        value.resize(size);
        std::memcpy(value.data(), data, size);
        data += size;
        labels.emplace(std::move(key), std::move(value));
    }
}

CONTRACT_NOINLINE void manual_write(const BenchMapPlain& record, unsigned char* data) {
    manual_write_map_like(record.labels, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchMapPlain& record) {
    manual_read_map_like(data, record.labels);
}

CONTRACT_NOINLINE void manual_write(const BenchMapHooked& record, unsigned char* data) {
    manual_write_map_like(record.labels, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchMapHooked& record) {
    manual_read_map_like(data, record.labels);
}

CONTRACT_NOINLINE void manual_write(const BenchUnorderedMapPlain& record, unsigned char* data) {
    manual_write_map_like(record.labels, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchUnorderedMapPlain& record) {
    manual_read_map_like(data, record.labels);
}

CONTRACT_NOINLINE void manual_write(const BenchUnorderedMapHooked& record, unsigned char* data) {
    manual_write_map_like(record.labels, data);
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchUnorderedMapHooked& record) {
    manual_read_map_like(data, record.labels);
}

CONTRACT_NOINLINE void manual_write(const BenchOptionalPlain& record, unsigned char* data) {
    const bool has_value = record.count.has_value();
    std::memcpy(data, &has_value, sizeof(has_value));
    data += sizeof(has_value);
    if (has_value) {
        const std::uint32_t value = *record.count;
        std::memcpy(data, &value, sizeof(value));
    }
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchOptionalPlain& record) {
    bool has_value = false;
    std::memcpy(&has_value, data, sizeof(has_value));
    data += sizeof(has_value);
    if (has_value) {
        std::uint32_t value = 0;
        std::memcpy(&value, data, sizeof(value));
        record.count = value;
    } else {
        record.count.reset();
    }
}

CONTRACT_NOINLINE void manual_write(const BenchOptionalHooked& record, unsigned char* data) {
    const bool has_value = record.count.has_value();
    std::memcpy(data, &has_value, sizeof(has_value));
    data += sizeof(has_value);
    if (has_value) {
        const std::uint32_t value = *record.count;
        std::memcpy(data, &value, sizeof(value));
    }
}

CONTRACT_NOINLINE void manual_read(const unsigned char* data, BenchOptionalHooked& record) {
    bool has_value = false;
    std::memcpy(&has_value, data, sizeof(has_value));
    data += sizeof(has_value);
    if (has_value) {
        std::uint32_t value = 0;
        std::memcpy(&value, data, sizeof(value));
        record.count = value;
    } else {
        record.count.reset();
    }
}

template<class Plain>
measurement_value write_ratio(
    const std::vector<std::size_t>& order,
    const Plain& source_a,
    const Plain& source_b,
    unsigned char* buffer,
    std::size_t buffer_size,
    volatile std::uint64_t& sink,
    int iteration_count = iterations)
{
    const std::array<Plain, 5> sources{source_a, source_b, source_a, source_b, source_a};
    return write_ratio(order, sources, buffer, buffer_size, sink, iteration_count);
}

template<class Plain>
measurement_value write_ratio(
    const std::vector<std::size_t>& order,
    const std::array<Plain, 5>& sources,
    unsigned char* buffer,
    std::size_t buffer_size,
    volatile std::uint64_t& sink,
    int iteration_count = iterations)
{
    return benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t slot) {
            const auto& source = sources[benchmark_base::selection_slot(order, i)];
            write_contract(source, buffer, buffer_size);
            if (buffer_size != 0) {
                benchmark_base::escape(buffer[0]);
            }
        },
        [&] {
            benchmark_base::consume_bytes(sink, buffer, buffer_size);
        },
        [&](int i, std::size_t slot) {
            const auto& source = sources[benchmark_base::selection_slot(order, i)];
            manual_write(source, buffer);
            if (buffer_size != 0) {
                benchmark_base::escape(buffer[0]);
            }
        },
        [&] {
            benchmark_base::consume_bytes(sink, buffer, buffer_size);
        });
}

template<class Plain>
measurement_value read_ratio_input(
    const std::vector<std::size_t>& order,
    const unsigned char* buffer,
    std::size_t size,
    Plain& target,
    volatile std::uint64_t& sink,
    int iteration_count = iterations) {
    return benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t) {
            read_contract_view(buffer, size, target);
        },
        [&] {
            benchmark_base::consume_object(sink, target);
        },
        [&](int, std::size_t) {
            manual_read(buffer, target);
        },
        [&] {
            benchmark_base::consume_object(sink, target);
        });
}

template<class Plain>
measurement_value read_ratio_copy(
    const std::vector<std::size_t>& order,
    const unsigned char* buffer,
    std::size_t size,
    Plain& target,
    volatile std::uint64_t& sink,
    int iteration_count = iterations) {
    return benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t) {
            read_contract_copy(buffer, size, target);
        },
        [&] {
            benchmark_base::consume_object(sink, target);
        },
        [&](int, std::size_t) {
            manual_read(buffer, target);
        },
        [&] {
            benchmark_base::consume_object(sink, target);
        });
}

template<bool HasRead, class Plain, class AssignPlain>
row_values single_field_values(
    const std::vector<std::size_t>& order,
    Plain& source,
    Plain& source_alt,
    Plain& read_target,
    Plain& plain_target,
    Plain& plain_manual_target,
    unsigned char* buffer,
    std::size_t buffer_size,
    AssignPlain&& assign_plain,
    volatile std::uint64_t& sink,
    int iteration_count = iterations)
{
    const std::array<Plain, 5> sources{source, source_alt, source, source_alt, source};
    return single_field_values<HasRead>(order, sources, read_target, plain_target, plain_manual_target,
        buffer, buffer_size, std::forward<AssignPlain>(assign_plain), sink, iteration_count);
}

template<bool HasRead, class Plain, class AssignPlain>
row_values single_field_values(
    const std::vector<std::size_t>& order,
    const std::array<Plain, 5>& sources,
    Plain& read_target,
    Plain& plain_target,
    Plain& plain_manual_target,
    unsigned char* buffer,
    std::size_t buffer_size,
    AssignPlain&& assign_plain,
    volatile std::uint64_t& sink,
    int iteration_count = iterations)
{
    row_values values{};
    auto plain_field = contract::field_at<0, Plain>();
    auto contract_get_value = plain_field.get(sources[0]);
    auto manual_get_value = plain_field.ref(sources[0]);
    auto use_in_measurement = [&](const auto& value) {
        using value_t = std::decay_t<decltype(value)>;
        if constexpr (std::is_integral_v<value_t> || std::is_enum_v<value_t>) {
            sink ^= static_cast<std::uint64_t>(value);
        } else {
            benchmark_base::escape(const_cast<value_t&>(value));
        }
    };

    values[out_obj] = write_ratio(order, sources, buffer, buffer_size, sink, iteration_count);

    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t slot) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
            use_in_measurement(contract_get_value);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t slot) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
            use_in_measurement(manual_get_value);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t slot) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
            use_in_measurement(contract_get_value);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t slot) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
            use_in_measurement(manual_get_value);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    if constexpr (HasRead) {
        write_contract(sources[0], buffer, buffer_size);
        values[in_obj_zero_copy] = read_ratio_input(order, buffer, buffer_size, read_target, sink, iteration_count);
        if constexpr (supports_copy_read_v<Plain>) {
            values[in_obj_copy] = read_ratio_copy(order, buffer, buffer_size, read_target, sink, iteration_count);
        }
    }

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        iteration_count,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t slot) {
            assign_plain(plain_target, i, slot, [&](auto&& value) {
                plain_field.set(plain_target, std::forward<decltype(value)>(value));
            });
            use_in_measurement(plain_field.ref(plain_target));
        },
        [&] {
            benchmark_base::consume_object(sink, plain_target);
        },
        [&](int i, std::size_t slot) {
            assign_plain(plain_manual_target, i, slot, [&](auto&& value) {
                plain_field.ref(plain_manual_target) = std::forward<decltype(value)>(value);
            });
            use_in_measurement(plain_field.ref(plain_manual_target));
        },
        [&] {
            benchmark_base::consume_object(sink, plain_manual_target);
        });

    finalize_row_state(buffer, buffer_size, sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target,
        plain_target, plain_manual_target);

    return values;
}

result_row make_numeric_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 4));
    BenchNumericPlain source;
    BenchNumericPlain source_alt;
    BenchNumericPlain source2;
    BenchNumericPlain source3;
    BenchNumericPlain source4;
    source.id = 100 + seed0;
    source.count = 42 + seed0;
    source.ratio = 3.25 + static_cast<double>(seed0);
    source.total = 9000 + seed0;
    source_alt.id = 101 + seed1;
    source_alt.count = 43 + seed1;
    source_alt.ratio = 4.25 + static_cast<double>(seed1);
    source_alt.total = 9001 + seed1;
    source2.id = 202 + seed2;
    source2.count = 44 + seed2;
    source2.ratio = 5.25 + static_cast<double>(seed2);
    source2.total = 9002 + seed2;
    source3.id = 303 + seed3;
    source3.count = 45 + seed3;
    source3.ratio = 6.25 + static_cast<double>(seed3);
    source3.total = 9003 + seed3;
    source4.id = 404 + seed4;
    source4.count = 46 + seed4;
    source4.ratio = 7.25 + static_cast<double>(seed4);
    source4.total = 9004 + seed4;
    BenchNumericPlain read_target;
    BenchNumericPlain set_target;
    BenchNumericPlain set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 2 + sizeof(std::uint32_t) + sizeof(double)> buffer{};
    row_values values{};
    auto sources = std::array{source, source_alt, source2, source3, source4};
    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto id = contract::field_at<0, BenchNumericPlain>();
    auto count = contract::field_at<1, BenchNumericPlain>();
    auto ratio_field = contract::field_at<2, BenchNumericPlain>();
    auto total = contract::field_at<3, BenchNumericPlain>();
    auto contract_get_value = id.get(source);
    auto manual_get_value = id.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = id.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = id.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = id.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = id.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            id.set(set_target, std::uint64_t(100 + seed));
            count.set(set_target, std::uint32_t(42 + seed));
            ratio_field.set(set_target, 3.25 + static_cast<double>(seed));
            total.set(set_target, std::uint64_t(9000 + seed));
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.id = std::uint64_t(100 + seed);
            set_manual_target.count = std::uint32_t(42 + seed);
            set_manual_target.ratio = 3.25 + static_cast<double>(seed);
            set_manual_target.total = std::uint64_t(9000 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });

    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);

    return {row_name("Numeric", "4 fields"), row_iterations, values};
}

result_row make_numeric_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 4));
    BenchNumericHooked source;
    BenchNumericHooked source_alt;
    BenchNumericHooked source2;
    BenchNumericHooked source3;
    BenchNumericHooked source4;
    source.id = 100 + seed0;
    source.count = 42 + seed0;
    source.ratio = 3.25 + static_cast<double>(seed0);
    source.total = 9000 + seed0;
    source_alt.id = 101 + seed1;
    source_alt.count = 43 + seed1;
    source_alt.ratio = 4.25 + static_cast<double>(seed1);
    source_alt.total = 9001 + seed1;
    source2.id = 202 + seed2;
    source2.count = 44 + seed2;
    source2.ratio = 5.25 + static_cast<double>(seed2);
    source2.total = 9002 + seed2;
    source3.id = 303 + seed3;
    source3.count = 45 + seed3;
    source3.ratio = 6.25 + static_cast<double>(seed3);
    source3.total = 9003 + seed3;
    source4.id = 404 + seed4;
    source4.count = 46 + seed4;
    source4.ratio = 7.25 + static_cast<double>(seed4);
    source4.total = 9004 + seed4;
    BenchNumericHooked read_target;
    BenchNumericHooked set_target;
    BenchNumericHooked set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 2 + sizeof(std::uint32_t) + sizeof(double)> buffer{};
    row_values values{};
    auto sources = std::array{source, source_alt, source2, source3, source4};
    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto id = contract::field_at<0, BenchNumericHooked>();
    auto count = contract::field_at<1, BenchNumericHooked>();
    auto ratio_field = contract::field_at<2, BenchNumericHooked>();
    auto total = contract::field_at<3, BenchNumericHooked>();
    auto contract_get_value = id.get(source);
    auto manual_get_value = id.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = id.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = id.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = id.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = id.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            id.set(set_target, std::uint64_t(100 + seed));
            count.set(set_target, std::uint32_t(42 + seed));
            ratio_field.set(set_target, 3.25 + static_cast<double>(seed));
            total.set(set_target, std::uint64_t(9000 + seed));
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.id = std::uint64_t(100 + seed);
            set_manual_target.count = std::uint32_t(42 + seed);
            set_manual_target.ratio = 3.25 + static_cast<double>(seed);
            set_manual_target.total = std::uint64_t(9000 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });

    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);

    return {row_name("Numeric", "4 fields, hooked"), row_iterations, values};
}

result_row make_string_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchStringPlain source;
    BenchStringPlain source_alt;
    BenchStringPlain source2;
    BenchStringPlain source3;
    BenchStringPlain source4;
    source.name = std::string(suite.text_size, 'c');
    source_alt.name = std::string(suite.text_size, 'a');
    source2.name = std::string(suite.text_size, 'b');
    source3.name = std::string(suite.text_size, 'd');
    source4.name = std::string(suite.text_size, 'e');
    BenchStringPlain read_target;
    BenchStringPlain set_target;
    BenchStringPlain set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    auto assign_plain = [text_size = suite.text_size](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::string(text_size, 'c')); break;
        case 1: set(std::string(text_size, 'a')); break;
        case 2: set(std::string(text_size, 'b')); break;
        case 3: set(std::string(text_size, 'd')); break;
        default: set(std::string(text_size, 'e')); break;
        }
    };

    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::string", std::string("len=") + std::to_string(suite.text_size)), row_iterations, values};
}

result_row make_string_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchStringHooked source;
    BenchStringHooked source_alt;
    BenchStringHooked source2;
    BenchStringHooked source3;
    BenchStringHooked source4;
    source.name = std::string(suite.text_size, 'c');
    source_alt.name = std::string(suite.text_size, 'a');
    source2.name = std::string(suite.text_size, 'b');
    source3.name = std::string(suite.text_size, 'd');
    source4.name = std::string(suite.text_size, 'e');
    BenchStringHooked read_target;
    BenchStringHooked set_target;
    BenchStringHooked set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    auto assign_plain = [text_size = suite.text_size](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::string(text_size, 'c')); break;
        case 1: set(std::string(text_size, 'a')); break;
        case 2: set(std::string(text_size, 'b')); break;
        case 3: set(std::string(text_size, 'd')); break;
        default: set(std::string(text_size, 'e')); break;
        }
    };

    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::string", std::string("len=") + std::to_string(suite.text_size) + ", hooked"), row_iterations, values};
}

result_row make_string_view_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchStringViewPlain source;
    BenchStringViewPlain source_alt;
    BenchStringViewPlain source2;
    BenchStringViewPlain source3;
    BenchStringViewPlain source4;
    std::string source_storage(suite.text_size, 'c');
    std::string source_alt_storage(suite.text_size, 'a');
    std::string source2_storage(suite.text_size, 'b');
    std::string source3_storage(suite.text_size, 'd');
    std::string source4_storage(suite.text_size, 'e');
    source.name = source_storage;
    source_alt.name = source_alt_storage;
    source2.name = source2_storage;
    source3.name = source3_storage;
    source4.name = source4_storage;
    BenchStringViewPlain read_target;
    BenchStringViewPlain set_target;
    BenchStringViewPlain set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    const std::string adapter(suite.text_size, 'a');
    const std::string contract(suite.text_size, 'c');
    auto assign_plain = [&](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::string_view{contract}); break;
        case 1: set(std::string_view{adapter}); break;
        case 2: set(std::string_view{std::string(suite.text_size, 'b')}); break;
        case 3: set(std::string_view{std::string(suite.text_size, 'd')}); break;
        default: set(std::string_view{std::string(suite.text_size, 'e')}); break;
        }
    };

    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::string_view", std::string("len=") + std::to_string(suite.text_size)), row_iterations, values};
}

result_row make_string_view_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchStringViewHooked source;
    BenchStringViewHooked source_alt;
    BenchStringViewHooked source2;
    BenchStringViewHooked source3;
    BenchStringViewHooked source4;
    std::string source_storage(suite.text_size, 'c');
    std::string source_alt_storage(suite.text_size, 'a');
    std::string source2_storage(suite.text_size, 'b');
    std::string source3_storage(suite.text_size, 'd');
    std::string source4_storage(suite.text_size, 'e');
    source.name = source_storage;
    source_alt.name = source_alt_storage;
    source2.name = source2_storage;
    source3.name = source3_storage;
    source4.name = source4_storage;
    BenchStringViewHooked read_target;
    BenchStringViewHooked set_target;
    BenchStringViewHooked set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    const std::string adapter(suite.text_size, 'a');
    const std::string contract(suite.text_size, 'c');
    auto assign_plain = [&](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::string_view{contract}); break;
        case 1: set(std::string_view{adapter}); break;
        case 2: set(std::string_view{std::string(suite.text_size, 'b')}); break;
        case 3: set(std::string_view{std::string(suite.text_size, 'd')}); break;
        default: set(std::string_view{std::string(suite.text_size, 'e')}); break;
        }
    };

    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::string_view", std::string("len=") + std::to_string(suite.text_size) + ", hooked"), row_iterations, values};
}

result_row make_cstring_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchCStringPlain source;
    BenchCStringPlain source_alt;
    BenchCStringPlain source2;
    BenchCStringPlain source3;
    BenchCStringPlain source4;
    std::string source_storage(suite.text_size, 'c');
    std::string source_alt_storage(suite.text_size, 'a');
    std::string source2_storage(suite.text_size, 'b');
    std::string source3_storage(suite.text_size, 'd');
    std::string source4_storage(suite.text_size, 'e');
    source.category = source_storage.c_str();
    source_alt.category = source_alt_storage.c_str();
    source2.category = source2_storage.c_str();
    source3.category = source3_storage.c_str();
    source4.category = source4_storage.c_str();
    BenchCStringPlain read_target;
    BenchCStringPlain set_target;
    BenchCStringPlain set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    const std::string adapter(suite.text_size, 'a');
    const std::string contract(suite.text_size, 'c');
    auto assign_plain = [&](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(contract.c_str()); break;
        case 1: set(adapter.c_str()); break;
        case 2: set("bbbbbbbbbbbbbbbb"); break;
        case 3: set("dddddddddddddddd"); break;
        default: set("eeeeeeeeeeeeeeee"); break;
        }
    };

    const auto values = single_field_values<false>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("const char*", std::string("len=") + std::to_string(suite.text_size)), row_iterations, values};
}

result_row make_cstring_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchCStringHooked source;
    BenchCStringHooked source_alt;
    BenchCStringHooked source2;
    BenchCStringHooked source3;
    BenchCStringHooked source4;
    std::string source_storage(suite.text_size, 'c');
    std::string source_alt_storage(suite.text_size, 'a');
    std::string source2_storage(suite.text_size, 'b');
    std::string source3_storage(suite.text_size, 'd');
    std::string source4_storage(suite.text_size, 'e');
    source.category = source_storage.c_str();
    source_alt.category = source_alt_storage.c_str();
    source2.category = source2_storage.c_str();
    source3.category = source3_storage.c_str();
    source4.category = source4_storage.c_str();
    BenchCStringHooked read_target;
    BenchCStringHooked set_target;
    BenchCStringHooked set_manual_target;
    std::vector<unsigned char> buffer(suite.text_size + sizeof(std::size_t) + 64);
    auto sources = std::array{source, source_alt, source2, source3, source4};

    const std::string adapter(suite.text_size, 'a');
    const std::string contract(suite.text_size, 'c');
    auto assign_plain = [&](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(contract.c_str()); break;
        case 1: set(adapter.c_str()); break;
        case 2: set("bbbbbbbbbbbbbbbb"); break;
        case 3: set("dddddddddddddddd"); break;
        default: set("eeeeeeeeeeeeeeee"); break;
        }
    };

    const auto values = single_field_values<false>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("const char*", std::string("len=") + std::to_string(suite.text_size) + ", hooked"), row_iterations, values};
}

template<class Plain, std::size_t N>
result_row make_char_array_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    Plain source;
    Plain source_alt;
    Plain source2;
    Plain source3;
    Plain source4;
    Plain read_target;
    Plain set_target;
    Plain set_manual_target;
    std::array<unsigned char, N> buffer{};
    const auto alt_a = make_char_payload<N>('c');
    const auto alt_b = make_char_payload<N>('a');
    const auto alt_c = make_char_payload<N>('b');
    const auto alt_d = make_char_payload<N>('d');
    const auto alt_e = make_char_payload<N>('e');
    std::memcpy(source.tag, alt_b.data(), sizeof(source.tag));
    std::memcpy(source_alt.tag, alt_a.data(), sizeof(source_alt.tag));
    std::memcpy(source2.tag, alt_c.data(), sizeof(source2.tag));
    std::memcpy(source3.tag, alt_d.data(), sizeof(source3.tag));
    std::memcpy(source4.tag, alt_e.data(), sizeof(source4.tag));
    auto sources = std::array{source, source_alt, source2, source3, source4};

    row_values values{};
    auto plain_field = contract::field_at<0, Plain>();
    const char* contract_get_value = plain_field.get(sources[0]);
    const char* manual_get_value = plain_field.ref(sources[0]);

    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t slot) {
            switch (slot % 5) {
            case 0: std::memcpy(plain_field.ref(set_target), alt_b.data(), sizeof(set_target.tag)); break;
            case 1: std::memcpy(plain_field.ref(set_target), alt_a.data(), sizeof(set_target.tag)); break;
            case 2: {
                const auto payload = make_char_payload<N>('b');
                std::memcpy(plain_field.ref(set_target), payload.data(), sizeof(set_target.tag));
                break;
            }
            case 3: {
                const auto payload = make_char_payload<N>('d');
                std::memcpy(plain_field.ref(set_target), payload.data(), sizeof(set_target.tag));
                break;
            }
            default: {
                const auto payload = make_char_payload<N>('e');
                std::memcpy(plain_field.ref(set_target), payload.data(), sizeof(set_target.tag));
                break;
            }
            }
        },
        [&] {
            consume(sink, set_target.tag);
        },
        [&](int i, std::size_t slot) {
            switch (slot % 5) {
            case 0: std::memcpy(set_manual_target.tag, alt_b.data(), sizeof(set_manual_target.tag)); break;
            case 1: std::memcpy(set_manual_target.tag, alt_a.data(), sizeof(set_manual_target.tag)); break;
            case 2: {
                const auto payload = make_char_payload<N>('b');
                std::memcpy(set_manual_target.tag, payload.data(), sizeof(set_manual_target.tag));
                break;
            }
            case 3: {
                const auto payload = make_char_payload<N>('d');
                std::memcpy(set_manual_target.tag, payload.data(), sizeof(set_manual_target.tag));
                break;
            }
            default: {
                const auto payload = make_char_payload<N>('e');
                std::memcpy(set_manual_target.tag, payload.data(), sizeof(set_manual_target.tag));
                break;
            }
            }
        },
        [&] {
            consume(sink, set_manual_target.tag);
        });
    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);
    return {row_name("char", N), row_iterations, values};
}

result_row make_request_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    BenchRequestEventPlain source;
    BenchRequestEventPlain source_alt;
    BenchRequestEventPlain source2;
    BenchRequestEventPlain source3;
    BenchRequestEventPlain source4;
    source_alt.timestamp = 8 + seed0;
    source_alt.service = 12 + seed0;
    source_alt.user_id = 43 + seed0;
    source2.timestamp = 18 + seed1;
    source2.service = 22 + seed1;
    source2.user_id = 53 + seed1;
    source3.timestamp = 28 + seed2;
    source3.service = 32 + seed2;
    source3.user_id = 63 + seed2;
    source4.timestamp = 38 + seed3;
    source4.service = 42 + seed3;
    source4.user_id = 73 + seed3;
    auto sources = std::array{source, source_alt, source2, source3, source4};
    BenchRequestEventPlain read_target;
    BenchRequestEventPlain set_target;
    BenchRequestEventPlain set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 3> buffer{};
    row_values values{};

    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto plain_field = contract::field_at<0, BenchRequestEventPlain>();
    auto contract_get_value = plain_field.get(source);
    auto manual_get_value = plain_field.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_target.timestamp = std::uint64_t(7 + seed);
            set_target.service = std::uint64_t(11 + seed);
            set_target.user_id = std::uint64_t(42 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.timestamp = std::uint64_t(7 + seed);
            set_manual_target.service = std::uint64_t(11 + seed);
            set_manual_target.user_id = std::uint64_t(42 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });
    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);
    return {row_name("RequestEvent", "3x u64"), row_iterations, values};
}

result_row make_request_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    BenchRequestEventHooked source;
    BenchRequestEventHooked source_alt;
    BenchRequestEventHooked source2;
    BenchRequestEventHooked source3;
    BenchRequestEventHooked source4;
    source_alt.timestamp = 8 + seed0;
    source_alt.service = 12 + seed0;
    source_alt.user_id = 43 + seed0;
    source2.timestamp = 18 + seed1;
    source2.service = 22 + seed1;
    source2.user_id = 53 + seed1;
    source3.timestamp = 28 + seed2;
    source3.service = 32 + seed2;
    source3.user_id = 63 + seed2;
    source4.timestamp = 38 + seed3;
    source4.service = 42 + seed3;
    source4.user_id = 73 + seed3;
    auto sources = std::array{source, source_alt, source2, source3, source4};
    BenchRequestEventHooked read_target;
    BenchRequestEventHooked set_target;
    BenchRequestEventHooked set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 3> buffer{};
    row_values values{};

    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto plain_field = contract::field_at<0, BenchRequestEventHooked>();
    auto contract_get_value = plain_field.get(source);
    auto manual_get_value = plain_field.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });

    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_target.timestamp = std::uint64_t(7 + seed);
            set_target.service = std::uint64_t(11 + seed);
            set_target.user_id = std::uint64_t(42 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.timestamp = std::uint64_t(7 + seed);
            set_manual_target.service = std::uint64_t(11 + seed);
            set_manual_target.user_id = std::uint64_t(42 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });
    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);
    return {row_name("RequestEvent", "3x u64, hooked"), row_iterations, values};
}

result_row make_routed_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    BenchRoutedEventPlain source;
    BenchRoutedEventPlain source_alt;
    BenchRoutedEventPlain source2;
    BenchRoutedEventPlain source3;
    BenchRoutedEventPlain source4;
    source_alt.timestamp = 8 + seed0;
    source_alt.service = 12 + seed0;
    source_alt.trace_id = 6 + seed0;
    source_alt.route_id = 10 + seed0;
    source2.timestamp = 18 + seed1;
    source2.service = 22 + seed1;
    source2.trace_id = 26 + seed1;
    source2.route_id = 30 + seed1;
    source3.timestamp = 28 + seed2;
    source3.service = 32 + seed2;
    source3.trace_id = 36 + seed2;
    source3.route_id = 40 + seed2;
    source4.timestamp = 38 + seed3;
    source4.service = 42 + seed3;
    source4.trace_id = 46 + seed3;
    source4.route_id = 50 + seed3;
    auto sources = std::array{source, source_alt, source2, source3, source4};
    BenchRoutedEventPlain read_target;
    BenchRoutedEventPlain set_target;
    BenchRoutedEventPlain set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 4> buffer{};
    row_values values{};

    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto plain_field = contract::field_at<0, BenchRoutedEventPlain>();
    auto contract_get_value = plain_field.get(source);
    auto manual_get_value = plain_field.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_target.timestamp = std::uint64_t(7 + seed);
            set_target.service = std::uint64_t(11 + seed);
            set_target.trace_id = std::uint64_t(5 + seed);
            set_target.route_id = std::uint64_t(9 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.timestamp = std::uint64_t(7 + seed);
            set_manual_target.service = std::uint64_t(11 + seed);
            set_manual_target.trace_id = std::uint64_t(5 + seed);
            set_manual_target.route_id = std::uint64_t(9 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });
    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);
    return {row_name("RoutedEvent", "4x u64"), row_iterations, values};
}

result_row make_routed_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint64_t>(benchmark_base::seed_at(order, 3));
    BenchRoutedEventHooked source;
    BenchRoutedEventHooked source_alt;
    BenchRoutedEventHooked source2;
    BenchRoutedEventHooked source3;
    BenchRoutedEventHooked source4;
    source_alt.timestamp = 8 + seed0;
    source_alt.service = 12 + seed0;
    source_alt.trace_id = 6 + seed0;
    source_alt.route_id = 10 + seed0;
    source2.timestamp = 18 + seed1;
    source2.service = 22 + seed1;
    source2.trace_id = 26 + seed1;
    source2.route_id = 30 + seed1;
    source3.timestamp = 28 + seed2;
    source3.service = 32 + seed2;
    source3.trace_id = 36 + seed2;
    source3.route_id = 40 + seed2;
    source4.timestamp = 38 + seed3;
    source4.service = 42 + seed3;
    source4.trace_id = 46 + seed3;
    source4.route_id = 50 + seed3;
    auto sources = std::array{source, source_alt, source2, source3, source4};
    BenchRoutedEventHooked read_target;
    BenchRoutedEventHooked set_target;
    BenchRoutedEventHooked set_manual_target;
    std::array<unsigned char, sizeof(std::uint64_t) * 4> buffer{};
    row_values values{};

    values[out_obj] = write_ratio(order, sources, buffer.data(), buffer.size(), sink, row_iterations);
    write_contract(sources[0], buffer.data(), buffer.size());
    values[in_obj_zero_copy] = read_ratio_input(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);
    values[in_obj_copy] = read_ratio_copy(order, buffer.data(), buffer.size(), read_target, sink, row_iterations);

    auto plain_field = contract::field_at<0, BenchRoutedEventHooked>();
    auto contract_get_value = plain_field.get(source);
    auto manual_get_value = plain_field.ref(source);
    values[field_get] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.get(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_ref] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            contract_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, contract_get_value);
        },
        [&](int i, std::size_t) {
            const auto& current = sources[benchmark_base::selection_slot(order, i)];
            manual_get_value = plain_field.ref(current);
        },
        [&] {
            consume(sink, manual_get_value);
        });
    values[field_set] = benchmark_base::loop_ratio(
        repeats,
        row_iterations,
        order,
        [](auto&& fn) {
            return benchmark_base::measure_cycles_once(std::forward<decltype(fn)>(fn));
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_target.timestamp = std::uint64_t(7 + seed);
            set_target.service = std::uint64_t(11 + seed);
            set_target.trace_id = std::uint64_t(5 + seed);
            set_target.route_id = std::uint64_t(9 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_target);
        },
        [&](int, std::size_t slot) {
            const auto seed = static_cast<std::uint64_t>(order[benchmark_base::selection_slot(order, slot)]);
            set_manual_target.timestamp = std::uint64_t(7 + seed);
            set_manual_target.service = std::uint64_t(11 + seed);
            set_manual_target.trace_id = std::uint64_t(5 + seed);
            set_manual_target.route_id = std::uint64_t(9 + seed);
        },
        [&] {
            benchmark_base::consume_object(sink, set_manual_target);
        });
    finalize_row_state(
        buffer.data(),
        buffer.size(),
        sink,
        sources[0],
        sources[1],
        sources[2],
        sources[3],
        sources[4],
        read_target,
        set_target,
        set_manual_target);
    return {row_name("RoutedEvent", "4x u64, hooked"), row_iterations, values};
}

result_row make_vector_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchVectorPlain source;
    BenchVectorPlain source_alt;
    BenchVectorPlain source2;
    BenchVectorPlain source3;
    BenchVectorPlain source4;
    source.values = make_u32_sequence(suite.vector_size, 1 + seed0);
    source_alt.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 1 + seed1));
    source2.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 2 + seed2));
    source3.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 3 + seed3));
    source4.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 4 + seed4));
    BenchVectorPlain read_target;
    BenchVectorPlain set_target;
    BenchVectorPlain set_manual_target;
    std::vector<unsigned char> buffer(suite.vector_size * sizeof(std::uint32_t) + sizeof(std::size_t) + 1024);
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_u32_sequence(suite.vector_size, 1 + static_cast<std::uint32_t>(slot))); break;
        case 1: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 1 + slot))); break;
        case 2: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 2 + slot))); break;
        case 3: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 3 + slot))); break;
        default: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 4 + slot))); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::vector<u32>", suite.vector_size), row_iterations, values};
}

result_row make_vector_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchVectorHooked source;
    BenchVectorHooked source_alt;
    BenchVectorHooked source2;
    BenchVectorHooked source3;
    BenchVectorHooked source4;
    source.values = make_u32_sequence(suite.vector_size, 1 + seed0);
    source_alt.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 1 + seed1));
    source2.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 2 + seed2));
    source3.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 3 + seed3));
    source4.values = make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 4 + seed4));
    BenchVectorHooked read_target;
    BenchVectorHooked set_target;
    BenchVectorHooked set_manual_target;
    std::vector<unsigned char> buffer(suite.vector_size * sizeof(std::uint32_t) + sizeof(std::size_t) + 1024);
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_u32_sequence(suite.vector_size, 1 + static_cast<std::uint32_t>(slot))); break;
        case 1: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 1 + slot))); break;
        case 2: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 2 + slot))); break;
        case 3: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 3 + slot))); break;
        default: set(make_u32_sequence(suite.vector_size, static_cast<std::uint32_t>(suite.vector_size + 4 + slot))); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::vector<u32>", std::string(std::to_string(suite.vector_size)) + ", hooked"), row_iterations, values};
}

template<class Plain, std::size_t N>
result_row make_array_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    Plain source;
    Plain source_alt;
    Plain source2;
    Plain source3;
    Plain source4;
    source.values = make_u32_array<N>(1 + seed0);
    source_alt.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 1 + seed1));
    source2.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 2 + seed2));
    source3.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 3 + seed3));
    source4.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 4 + seed4));
    Plain read_target;
    Plain set_target;
    Plain set_manual_target;
    std::array<unsigned char, N * sizeof(std::uint32_t) + 64> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_u32_array<N>(1 + static_cast<std::uint32_t>(slot))); break;
        case 1: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 1 + slot))); break;
        case 2: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 2 + slot))); break;
        case 3: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 3 + slot))); break;
        default: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 4 + slot))); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::array<u32>", N), row_iterations, values};
}

template<class Plain, std::size_t N>
result_row make_array_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    Plain source;
    Plain source_alt;
    Plain source2;
    Plain source3;
    Plain source4;
    source.values = make_u32_array<N>(1 + seed0);
    source_alt.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 1 + seed1));
    source2.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 2 + seed2));
    source3.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 3 + seed3));
    source4.values = make_u32_array<N>(static_cast<std::uint32_t>(N + 4 + seed4));
    Plain read_target;
    Plain set_target;
    Plain set_manual_target;
    std::array<unsigned char, N * sizeof(std::uint32_t) + 64> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_u32_array<N>(1 + static_cast<std::uint32_t>(slot))); break;
        case 1: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 1 + slot))); break;
        case 2: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 2 + slot))); break;
        case 3: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 3 + slot))); break;
        default: set(make_u32_array<N>(static_cast<std::uint32_t>(N + 4 + slot))); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::array<u32>", std::string(std::to_string(N)) + ", hooked"), row_iterations, values};
}

result_row make_tuple_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = benchmark_base::seed_at(order, 0);
    const auto seed1 = benchmark_base::seed_at(order, 1);
    const auto seed2 = benchmark_base::seed_at(order, 2);
    const auto seed3 = benchmark_base::seed_at(order, 3);
    const auto seed4 = benchmark_base::seed_at(order, 4);
    BenchTuplePlain source;
    BenchTuplePlain source_alt;
    BenchTuplePlain source2;
    BenchTuplePlain source3;
    BenchTuplePlain source4;
    source.payload = make_tuple_payload(seed0);
    source_alt.payload = make_tuple_payload(seed1);
    source2.payload = make_tuple_payload(seed2);
    source3.payload = make_tuple_payload(seed3);
    source4.payload = make_tuple_payload(seed4);
    BenchTuplePlain read_target;
    BenchTuplePlain set_target;
    BenchTuplePlain set_manual_target;
    std::array<unsigned char, 256> buffer{};
    auto assign_plain = [&order](auto&, int, std::size_t slot, auto&& set) {
        set(make_tuple_payload(slot + order[benchmark_base::selection_slot(order, slot)]));
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::tuple<u32,string,array<u32,3>>", "1"), row_iterations, values};
}

result_row make_tuple_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = benchmark_base::seed_at(order, 0);
    const auto seed1 = benchmark_base::seed_at(order, 1);
    const auto seed2 = benchmark_base::seed_at(order, 2);
    const auto seed3 = benchmark_base::seed_at(order, 3);
    const auto seed4 = benchmark_base::seed_at(order, 4);
    BenchTupleHooked source;
    BenchTupleHooked source_alt;
    BenchTupleHooked source2;
    BenchTupleHooked source3;
    BenchTupleHooked source4;
    source.payload = make_tuple_payload(seed0);
    source_alt.payload = make_tuple_payload(seed1);
    source2.payload = make_tuple_payload(seed2);
    source3.payload = make_tuple_payload(seed3);
    source4.payload = make_tuple_payload(seed4);
    BenchTupleHooked read_target;
    BenchTupleHooked set_target;
    BenchTupleHooked set_manual_target;
    std::array<unsigned char, 256> buffer{};
    auto assign_plain = [&order](auto&, int, std::size_t slot, auto&& set) {
        set(make_tuple_payload(slot + order[benchmark_base::selection_slot(order, slot)]));
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::tuple<u32,string,array<u32,3>>", "1, hooked"), row_iterations, values};
}

result_row make_variant_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = benchmark_base::seed_at(order, 0);
    const auto seed1 = benchmark_base::seed_at(order, 1);
    const auto seed2 = benchmark_base::seed_at(order, 2);
    const auto seed3 = benchmark_base::seed_at(order, 3);
    const auto seed4 = benchmark_base::seed_at(order, 4);
    BenchVariantPlain source;
    BenchVariantPlain source_alt;
    BenchVariantPlain source2;
    BenchVariantPlain source3;
    BenchVariantPlain source4;
    source.payload = make_variant_payload(seed0);
    source_alt.payload = make_variant_payload(seed1);
    source2.payload = make_variant_payload(seed2);
    source3.payload = make_variant_payload(seed3);
    source4.payload = make_variant_payload(seed4);
    BenchVariantPlain read_target;
    BenchVariantPlain set_target;
    BenchVariantPlain set_manual_target;
    std::array<unsigned char, 256> buffer{};
    auto assign_plain = [&order](auto&, int, std::size_t slot, auto&& set) {
        set(make_variant_payload(slot + order[benchmark_base::selection_slot(order, slot)]));
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::variant<u32,string,array<u32,3>>", "1"), row_iterations, values};
}

result_row make_variant_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = benchmark_base::seed_at(order, 0);
    const auto seed1 = benchmark_base::seed_at(order, 1);
    const auto seed2 = benchmark_base::seed_at(order, 2);
    const auto seed3 = benchmark_base::seed_at(order, 3);
    const auto seed4 = benchmark_base::seed_at(order, 4);
    BenchVariantHooked source;
    BenchVariantHooked source_alt;
    BenchVariantHooked source2;
    BenchVariantHooked source3;
    BenchVariantHooked source4;
    source.payload = make_variant_payload(seed0);
    source_alt.payload = make_variant_payload(seed1);
    source2.payload = make_variant_payload(seed2);
    source3.payload = make_variant_payload(seed3);
    source4.payload = make_variant_payload(seed4);
    BenchVariantHooked read_target;
    BenchVariantHooked set_target;
    BenchVariantHooked set_manual_target;
    std::array<unsigned char, 256> buffer{};
    auto assign_plain = [&order](auto&, int, std::size_t slot, auto&& set) {
        set(make_variant_payload(slot + order[benchmark_base::selection_slot(order, slot)]));
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::variant<u32,string,array<u32,3>>", "1, hooked"), row_iterations, values};
}

result_row make_map_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchMapPlain source;
    BenchMapPlain source_alt;
    BenchMapPlain source2;
    BenchMapPlain source3;
    BenchMapPlain source4;
    source.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 1 + seed0, suite.text_size);
    source_alt.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 1001 + seed1, suite.text_size);
    source2.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 2001 + seed2, suite.text_size);
    source3.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 3001 + seed3, suite.text_size);
    source4.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 4001 + seed4, suite.text_size);
    BenchMapPlain read_target;
    BenchMapPlain set_target;
    BenchMapPlain set_manual_target;
    std::vector<unsigned char> buffer(
        suite.map_size * (suite.text_size + 256) + 4096);
    auto sources = std::array{source, source_alt, source2, source3, source4};
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 1 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 1: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 1001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 2: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 2001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 3: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 3001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        default: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 4001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        }
    };
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::map<u32,string>", suite.map_size, std::string("len=") + std::to_string(suite.text_size)), row_iterations, values};
}

result_row make_map_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchMapHooked source;
    BenchMapHooked source_alt;
    BenchMapHooked source2;
    BenchMapHooked source3;
    BenchMapHooked source4;
    source.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 1 + seed0, suite.text_size);
    source_alt.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 1001 + seed1, suite.text_size);
    source2.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 2001 + seed2, suite.text_size);
    source3.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 3001 + seed3, suite.text_size);
    source4.labels = make_label_map<std::map<std::uint32_t, std::string>>(
        suite.map_size, 4001 + seed4, suite.text_size);
    BenchMapHooked read_target;
    BenchMapHooked set_target;
    BenchMapHooked set_manual_target;
    std::vector<unsigned char> buffer(
        suite.map_size * (suite.text_size + 256) + 4096);
    auto sources = std::array{source, source_alt, source2, source3, source4};
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 1 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 1: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 1001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 2: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 2001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        case 3: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 3001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        default: set(make_label_map<std::map<std::uint32_t, std::string>>(suite.map_size, 4001 + static_cast<std::uint32_t>(slot), suite.text_size)); break;
        }
    };
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::map<u32,string>", suite.map_size, std::string("len=") + std::to_string(suite.text_size) + ", hooked"), row_iterations, values};
}

result_row make_optional_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchOptionalPlain source;
    BenchOptionalPlain source_alt;
    BenchOptionalPlain source2;
    BenchOptionalPlain source3;
    BenchOptionalPlain source4;
    source.count = 42 + seed0;
    source_alt.count = 44 + seed1;
    source2.count = 46 + seed2;
    source3.count = 48 + seed3;
    source4.count = 50 + seed4;
    BenchOptionalPlain read_target;
    BenchOptionalPlain set_target;
    BenchOptionalPlain set_manual_target;
    std::array<unsigned char, 64> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::optional<std::uint32_t>{42u + static_cast<std::uint32_t>(slot)}); break;
        case 1: set(std::optional<std::uint32_t>{44u + static_cast<std::uint32_t>(slot)}); break;
        case 2: set(std::optional<std::uint32_t>{46u + static_cast<std::uint32_t>(slot)}); break;
        case 3: set(std::optional<std::uint32_t>{48u + static_cast<std::uint32_t>(slot)}); break;
        default: set(std::optional<std::uint32_t>{50u + static_cast<std::uint32_t>(slot)}); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::optional<u32>", "1"), row_iterations, values};
}

result_row make_optional_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    const auto seed0 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 0));
    const auto seed1 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 1));
    const auto seed2 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 2));
    const auto seed3 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 3));
    const auto seed4 = static_cast<std::uint32_t>(benchmark_base::seed_at(order, 4));
    BenchOptionalHooked source;
    BenchOptionalHooked source_alt;
    BenchOptionalHooked source2;
    BenchOptionalHooked source3;
    BenchOptionalHooked source4;
    source.count = 42 + seed0;
    source_alt.count = 44 + seed1;
    source2.count = 46 + seed2;
    source3.count = 48 + seed3;
    source4.count = 50 + seed4;
    BenchOptionalHooked read_target;
    BenchOptionalHooked set_target;
    BenchOptionalHooked set_manual_target;
    std::array<unsigned char, 64> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(std::optional<std::uint32_t>{42u + static_cast<std::uint32_t>(slot)}); break;
        case 1: set(std::optional<std::uint32_t>{44u + static_cast<std::uint32_t>(slot)}); break;
        case 2: set(std::optional<std::uint32_t>{46u + static_cast<std::uint32_t>(slot)}); break;
        case 3: set(std::optional<std::uint32_t>{48u + static_cast<std::uint32_t>(slot)}); break;
        default: set(std::optional<std::uint32_t>{50u + static_cast<std::uint32_t>(slot)}); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::optional<u32>", "1, hooked"), row_iterations, values};
}

template<class Plain, std::size_t N>
result_row make_bitset_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    Plain source;
    Plain source_alt;
    Plain source2;
    Plain source3;
    Plain source4;
    source.flags = make_bitset_pattern<N>(true);
    source_alt.flags = make_bitset_pattern<N>(false);
    source2.flags = make_bitset_pattern<N>(true);
    source3.flags = make_bitset_pattern<N>(false);
    source4.flags = make_bitset_pattern<N>(true);
    Plain read_target;
    Plain set_target;
    Plain set_manual_target;
    std::array<unsigned char, (N <= 64 ? sizeof(std::uint64_t) : (N + 7) / 8)> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_bitset_pattern<N>(true)); break;
        case 1: set(make_bitset_pattern<N>(false)); break;
        case 2: set(make_bitset_pattern<N>(true)); break;
        case 3: set(make_bitset_pattern<N>(false)); break;
        default: set(make_bitset_pattern<N>(true)); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::bitset", N), row_iterations, values};
}

template<class Plain, std::size_t N>
result_row make_bitset_hooked_row(const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    Plain source;
    Plain source_alt;
    Plain source2;
    Plain source3;
    Plain source4;
    source.flags = make_bitset_pattern<N>(true);
    source_alt.flags = make_bitset_pattern<N>(false);
    source2.flags = make_bitset_pattern<N>(true);
    source3.flags = make_bitset_pattern<N>(false);
    source4.flags = make_bitset_pattern<N>(true);
    Plain read_target;
    Plain set_target;
    Plain set_manual_target;
    std::array<unsigned char, (N <= 64 ? sizeof(std::uint64_t) : (N + 7) / 8)> buffer{};
    auto assign_plain = [](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_bitset_pattern<N>(true)); break;
        case 1: set(make_bitset_pattern<N>(false)); break;
        case 2: set(make_bitset_pattern<N>(true)); break;
        case 3: set(make_bitset_pattern<N>(false)); break;
        default: set(make_bitset_pattern<N>(true)); break;
        }
    };
    auto sources = std::array{source, source_alt, source2, source3, source4};
    const auto values = single_field_values<true>(order, sources, read_target, set_target, set_manual_target,
        buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::bitset", std::string(std::to_string(N)) + ", hooked"), row_iterations, values};
}

result_row make_unordered_map_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchUnorderedMapPlain source;
    BenchUnorderedMapPlain source_alt;
    BenchUnorderedMapPlain source2;
    BenchUnorderedMapPlain source3;
    BenchUnorderedMapPlain source4;
    source.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 1, suite.text_size);
    source_alt.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 1001, suite.text_size);
    source2.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 2001, suite.text_size);
    source3.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 3001, suite.text_size);
    source4.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 4001, suite.text_size);
    BenchUnorderedMapPlain read_target;
    BenchUnorderedMapPlain set_target;
    BenchUnorderedMapPlain set_manual_target;
    std::vector<unsigned char> buffer(
        suite.unordered_map_size * (suite.text_size + 256) + 1024);
    auto sources = std::array{source, source_alt, source2, source3, source4};
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 1, suite.text_size)); break;
        case 1: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 1001, suite.text_size)); break;
        case 2: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 2001, suite.text_size)); break;
        case 3: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 3001, suite.text_size)); break;
        default: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 4001, suite.text_size)); break;
        }
    };
    const auto values = single_field_values<true>(order, sources, read_target, set_target,
        set_manual_target, buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::unordered_map<u32,string>", suite.unordered_map_size, std::string("len=") + std::to_string(suite.text_size)), row_iterations, values};
}

result_row make_unordered_map_hooked_row(const suite_config& suite, const std::vector<std::size_t>& order, volatile std::uint64_t& sink, int row_iterations) {
    BenchUnorderedMapHooked source;
    BenchUnorderedMapHooked source_alt;
    BenchUnorderedMapHooked source2;
    BenchUnorderedMapHooked source3;
    BenchUnorderedMapHooked source4;
    source.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 1, suite.text_size);
    source_alt.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 1001, suite.text_size);
    source2.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 2001, suite.text_size);
    source3.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 3001, suite.text_size);
    source4.labels = make_label_map<std::unordered_map<std::uint32_t, std::string>>(
        suite.unordered_map_size, 4001, suite.text_size);
    BenchUnorderedMapHooked read_target;
    BenchUnorderedMapHooked set_target;
    BenchUnorderedMapHooked set_manual_target;
    std::vector<unsigned char> buffer(
        suite.unordered_map_size * (suite.text_size + 256) + 1024);
    auto sources = std::array{source, source_alt, source2, source3, source4};
    auto assign_plain = [suite](auto&, int, std::size_t slot, auto&& set) {
        switch (slot % 5) {
        case 0: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 1, suite.text_size)); break;
        case 1: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 1001, suite.text_size)); break;
        case 2: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 2001, suite.text_size)); break;
        case 3: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 3001, suite.text_size)); break;
        default: set(make_label_map<std::unordered_map<std::uint32_t, std::string>>(suite.unordered_map_size, 4001, suite.text_size)); break;
        }
    };
    const auto values = single_field_values<true>(order, sources, read_target, set_target,
        set_manual_target, buffer.data(), buffer.size(), assign_plain, sink, row_iterations);
    finalize_row_state(buffer.data(), buffer.size(), sink, sources[0], sources[1], sources[2], sources[3], sources[4], read_target, set_target, set_manual_target);
    return {row_name("std::unordered_map<u32,string>", suite.unordered_map_size, std::string("len=") + std::to_string(suite.text_size) + ", hooked"), row_iterations, values};
}

struct row_sort_key {
    std::string base;
    std::size_t size = 0;
    std::string suffix;
};

struct row_entry {
    std::string name;
    std::function<result_row()> build;
};

struct cli_config {
    bool color = false;
    bool show_quantiles = false;
    bool list_rows = false;
    std::optional<std::size_t> row_index;
    std::optional<std::string> row_name;
};

void print_usage(std::ostream& out, const char* argv0) {
    out << "usage: " << argv0
        << " [--color] [--quantiles] [--iterations N]"
        << " [--list-rows]"
        << " [--row-index N | --row NAME]\n";
}

cli_config parse_cli(int argc, char** argv) {
    cli_config config;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--color") {
            config.color = true;
        } else if (arg == "--quantiles") {
            config.show_quantiles = true;
        } else if (arg == "--help" || arg == "-h") {
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

void print_rows(const std::vector<result_row>& rows, bool color, bool show_quantiles) {
    const benchmark_base::table_spec<column_count> spec{
        "Binary adapter ratio (contract/manual)",
        {"out", "get", "ref", "in(view)", "in(copy)", "set"},
        field_ref,
        static_cast<std::size_t>(show_quantiles ? 9 : 16),
        static_cast<std::size_t>(show_quantiles ? 1 : 3),
    };

    benchmark_base::render_table(rows, spec, color, show_quantiles, std::cout);
}

void print_rows_with_index(const std::vector<result_row>& rows, bool color, bool show_quantiles) {
    std::cout << "row index legend:\n";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        std::cout << "  " << (i + 1) << ": " << rows[i].name << "\n";
    }
    print_rows(rows, color, show_quantiles);
}

void prefix_row_names(std::vector<result_row>& rows, std::size_t first_index = 1) {
    for (std::size_t i = 0; i < rows.size(); ++i) {
        auto& name = rows[i].name;
        name = std::to_string(first_index + i) + ": " + name;
    }
}

std::vector<row_entry> make_row_entries(
    const std::vector<std::size_t>& order,
    volatile std::uint64_t& sink,
    int boosted_iterations,
    int reduced_iterations,
    const suite_config& small_suite,
    const suite_config& big_suite)
{
    std::vector<row_entry> rows;
    rows.reserve(48);
    rows.push_back({"Numeric[4 fields]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_numeric_row(order, sink, boosted_iterations); }});
    rows.push_back({"Numeric[4 fields, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_numeric_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({"Request[event]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_request_row(order, sink, boosted_iterations); }});
    rows.push_back({"Request[event, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_request_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({"Routed[event]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_routed_row(order, sink, boosted_iterations); }});
    rows.push_back({"Routed[event, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_routed_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({"Optional[value]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_optional_row(order, sink, boosted_iterations); }});
    rows.push_back({"Optional[value, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_optional_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({"Bitset[10]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_bitset_row<BenchBitsetPlain10, 10>(order, sink, boosted_iterations); }});
    rows.push_back({"Bitset[10, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_bitset_hooked_row<BenchBitsetHooked10, 10>(order, sink, boosted_iterations); }});
    rows.push_back({"Bitset[100]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_bitset_row<BenchBitsetPlain100, 100>(order, sink, boosted_iterations); }});
    rows.push_back({"Bitset[100, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_bitset_hooked_row<BenchBitsetHooked100, 100>(order, sink, boosted_iterations); }});
    rows.push_back({"CharArray[8]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_char_array_row<BenchCharArrayPlain8, 8>(order, sink, boosted_iterations); }});
    rows.push_back({"CharArray[1000]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_char_array_row<BenchCharArrayPlain1000, 1000>(order, sink, boosted_iterations); }});
    rows.push_back({"Array[4]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_array_row<BenchArrayPlain4, 4>(order, sink, boosted_iterations); }});
    rows.push_back({"Array[4, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_array_hooked_row<BenchArrayHooked4, 4>(order, sink, boosted_iterations); }});
    rows.push_back({"Array[1000]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_array_row<BenchArrayPlain1000, 1000>(order, sink, reduced_iterations); }});
    rows.push_back({"Array[1000, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_array_hooked_row<BenchArrayHooked1000, 1000>(order, sink, reduced_iterations); }});
    rows.push_back({"Tuple[value]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_tuple_row(order, sink, boosted_iterations); }});
    rows.push_back({"Tuple[value, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_tuple_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({"Variant[value]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_variant_row(order, sink, boosted_iterations); }});
    rows.push_back({"Variant[value, hooked]", [&, boosted_iterations, reduced_iterations, small_suite, big_suite] { return make_variant_hooked_row(order, sink, boosted_iterations); }});
    rows.push_back({row_name("std::string", std::string("len=") + std::to_string(small_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::string", std::string("len=") + std::to_string(small_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::string_view", std::string("len=") + std::to_string(small_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_view_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::string_view", std::string("len=") + std::to_string(small_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_view_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("const char*", std::string("len=") + std::to_string(small_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_cstring_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("const char*", std::string("len=") + std::to_string(small_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_cstring_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::vector<u32>", small_suite.vector_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_vector_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::vector<u32>", small_suite.vector_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_vector_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::map<u32,string>", small_suite.map_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_map_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::map<u32,string>", small_suite.map_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_map_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::unordered_map<u32,string>", small_suite.unordered_map_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_unordered_map_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::unordered_map<u32,string>", small_suite.unordered_map_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_unordered_map_hooked_row(small_suite, order, sink, boosted_iterations);
    }});
    rows.push_back({row_name("std::string", std::string("len=") + std::to_string(big_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("std::string", std::string("len=") + std::to_string(big_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_hooked_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("std::string_view", std::string("len=") + std::to_string(big_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_view_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("std::string_view", std::string("len=") + std::to_string(big_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_string_view_hooked_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("const char*", std::string("len=") + std::to_string(big_suite.text_size)), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_cstring_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("const char*", std::string("len=") + std::to_string(big_suite.text_size) + ", hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_cstring_hooked_row(big_suite, order, sink, iterations);
    }});
    rows.push_back({row_name("std::vector<u32>", big_suite.vector_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_vector_row(big_suite, order, sink, reduced_iterations);
    }});
    rows.push_back({row_name("std::vector<u32>", big_suite.vector_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_vector_hooked_row(big_suite, order, sink, reduced_iterations);
    }});
    rows.push_back({row_name("std::map<u32,string>", big_suite.map_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_map_row(big_suite, order, sink, reduced_iterations);
    }});
    rows.push_back({row_name("std::map<u32,string>", big_suite.map_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_map_hooked_row(big_suite, order, sink, reduced_iterations);
    }});
    rows.push_back({row_name("std::unordered_map<u32,string>", big_suite.unordered_map_size), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_unordered_map_row(big_suite, order, sink, reduced_iterations);
    }});
    rows.push_back({row_name("std::unordered_map<u32,string>", big_suite.unordered_map_size, "hooked"), [&, boosted_iterations, reduced_iterations, small_suite, big_suite] {
        return make_unordered_map_hooked_row(big_suite, order, sink, reduced_iterations);
    }});
    return rows;
}

} // namespace

int main(int argc, char** argv) {
    const auto cli = parse_cli(argc, argv);
    iterations = benchmark_base::parse_iterations_arg(argc, argv, iterations);

    volatile std::uint64_t sink = 0;
    const auto order = benchmark_base::make_order(static_cast<std::size_t>(iterations), 5);
    const int boosted_iterations = iterations * 10;
    const int reduced_iterations = std::max(1, iterations / 10);

    std::cout << "iterations: " << iterations
              << " repeats: " << repeats
              << " primary metric: cycles/op\n";

    constexpr suite_config small_suite{8, 4, 2, 2};
    constexpr suite_config big_suite{1000, 1000, 64, 64};

    auto rows = make_row_entries(order, sink, boosted_iterations, reduced_iterations, small_suite, big_suite);
    std::sort(rows.begin(), rows.end(), [](const row_entry& a, const row_entry& b) {
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

    if (cli.list_rows) {
        std::cout << "row index legend:\n";
        for (std::size_t i = 0; i < rows.size(); ++i) {
            std::cout << "  " << (i + 1) << ": " << rows[i].name << "\n";
        }
        return 0;
    }

    if (cli.row_index.has_value() || cli.row_name.has_value()) {
        std::vector<row_entry> selected;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto index = i + 1;
            const bool match_index = cli.row_index.has_value() && *cli.row_index == index;
            const bool match_name = cli.row_name.has_value() && rows[i].name == *cli.row_name;
            if (match_index || match_name) {
                selected.push_back(rows[i]);
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

        std::vector<result_row> results;
        results.reserve(selected.size());
        for (auto& entry : selected) {
            results.push_back(entry.build());
        }
        prefix_row_names(results, cli.row_index.has_value() ? *cli.row_index : 1);
        print_rows(results, cli.color, cli.show_quantiles);
        exercise_hooked_fixtures(sink);
        std::cout << "\nsink: " << sink << "\n";
        std::cout << "buffer version: 0x" << std::hex << buffer_version_sink << std::dec << "\n";
        return 0;
    }

    std::vector<result_row> results;
    results.reserve(rows.size());
    for (auto& entry : rows) {
        results.push_back(entry.build());
    }
    prefix_row_names(results);
    print_rows(results, cli.color, cli.show_quantiles);
    exercise_hooked_fixtures(sink);
    std::cout << "\nsink: " << sink << "\n";
    std::cout << "buffer version: 0x" << std::hex << buffer_version_sink << std::dec << "\n";
}
