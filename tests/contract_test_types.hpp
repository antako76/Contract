#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace contract_tests {

struct BaseCounters {
    volatile unsigned long long count = 10;
    volatile unsigned long long error = 2;
    volatile unsigned long long time = 30;
    volatile unsigned long long clock = 40;

    CONTRACT(BaseCounters, (count, 1), (error, 2), (time, 3), (clock, 4))
};

struct CountingAdapter {
    int fields = 0;
    int id_sum = 0;

    template<class Field, class Value>
    void field(const Field&, const Value&) {
        ++fields;
        id_sum += Field::id;
    }
};

struct DebugStringAdapter {
    explicit DebugStringAdapter(std::ostream& out)
        : out(out) {}

    template<class Field, class Object>
    void field(const Field& field, const Object& obj) {
        if (!first) {
            out << ", ";
        }

        first = false;
        out << field.name << '=' << field.get(obj);
    }

    std::ostream& out;
    bool first = true;
};

template<class Object>
std::string debug_string(const Object& obj) {
    std::ostringstream out;
    DebugStringAdapter adapter(out);
    contract::visit(obj, adapter);
    return out.str();
}

struct RequestBase {
    int timestamp = 7;
    int service = 11;

    CONTRACT(RequestBase, (timestamp, 1), (service, 2))
};

struct RequestEvent : public RequestBase {
    int user_id = 42;

    CONTRACT(RequestEvent, BASE(RequestBase, 1000), (user_id, 1101))
};

struct TraceContext {
    int trace_id = 5;

    CONTRACT(TraceContext, (trace_id, 1))
};

struct RoutedEvent : public RequestBase, public TraceContext {
    int route_id = 9;

    CONTRACT(RoutedEvent,
        BASE(RequestBase, 1000),
        BASE(TraceContext, 2000),
        (route_id, 3001))
};

struct HookedMetric {
    int raw_count = 0;

    CONTRACT(HookedMetric, (raw_count, 1))

    int contract_get(contract::tag<field::raw_count>) const {
        return raw_count * 10;
    }

    template<class Value>
    void contract_set(contract::tag<field::raw_count>, Value&& value) {
        raw_count = static_cast<int>(std::forward<Value>(value)) / 10;
    }
};

struct PlainMetric {
    int raw_count = 0;

    CONTRACT(PlainMetric, (raw_count, 1))
};

struct RequestStat {
    std::uint64_t started_ns = 100;
    std::uint64_t finished_ns = 150;

    CONTRACT(RequestStat,
        PROPERTY(duration_ns, 10, std::uint64_t),
        (started_ns, 11),
        (finished_ns, 12))

    std::uint64_t contract_get(contract::tag<field::duration_ns>) const {
        return finished_ns - started_ns;
    }

    void contract_set(contract::tag<field::duration_ns>, std::uint64_t value) {
        finished_ns = started_ns + value;
    }
};

struct ReferenceFallbackMetric {
    std::uint64_t storage = 0;
    std::uint64_t& raw_count;

    ReferenceFallbackMetric()
        : raw_count(storage) {}

    CONTRACT(ReferenceFallbackMetric, (raw_count, 1))
};

struct ReferenceHookMetric {
    std::uint64_t storage = 0;
    std::uint64_t& raw_count;

    ReferenceHookMetric()
        : raw_count(storage) {}

    CONTRACT(ReferenceHookMetric, (raw_count, 1))

    std::uint64_t contract_get(contract::tag<field::raw_count>) const {
        return raw_count * 10;
    }

    template<class Value>
    void contract_set(contract::tag<field::raw_count>, Value&& value) {
        raw_count = static_cast<std::uint64_t>(std::forward<Value>(value)) / 10;
    }
};

struct ReferenceFreeMetric {
    std::uint64_t storage = 0;
    std::uint64_t& raw_count;

    ReferenceFreeMetric()
        : raw_count(storage) {}

    CONTRACT(ReferenceFreeMetric, (raw_count, 1))
};

template<class Field>
std::uint64_t contract_get(const Field&, const ReferenceFreeMetric& metric) {
    static_assert(Field::id == 1);
    return metric.raw_count * 100;
}

template<class Field, class Value>
void contract_set(const Field&, ReferenceFreeMetric& metric, Value&& value) {
    static_assert(Field::id == 1);
    metric.raw_count = static_cast<std::uint64_t>(std::forward<Value>(value)) / 100;
}

struct FreeHookMetric {
    int raw_count = 0;

    CONTRACT(FreeHookMetric, (raw_count, 1))
};

template<class Field>
int contract_get(const Field&, const FreeHookMetric& metric) {
    static_assert(Field::id == 1);
    return metric.raw_count * 100;
}

template<class Field, class Value>
void contract_set(const Field&, FreeHookMetric& metric, Value&& value) {
    static_assert(Field::id == 1);
    metric.raw_count = static_cast<int>(std::forward<Value>(value)) / 100;
}

struct FreeTagMetric {
    int raw_count = 0;

    CONTRACT(FreeTagMetric, (raw_count, 1))
};

inline int contract_get(contract::tag<FreeTagMetric::field::raw_count>, const FreeTagMetric& metric) {
    return metric.raw_count * 1000;
}

template<class Value>
inline void contract_set(contract::tag<FreeTagMetric::field::raw_count>, FreeTagMetric& metric, Value&& value) {
    metric.raw_count = static_cast<int>(std::forward<Value>(value)) / 1000;
}

struct FreePropertyStat {
    std::uint64_t low = 20;
    std::uint64_t high = 45;

    CONTRACT(FreePropertyStat,
        PROPERTY(delta, 5, std::uint64_t),
        (low, 6),
        (high, 7))
};

template<class Field, std::enable_if_t<std::is_same_v<typename Field::tag, FreePropertyStat::field::delta>, int> = 0>
std::uint64_t contract_get(const Field&, const FreePropertyStat& stat) {
    return stat.high - stat.low;
}

template<class Field, class Value, std::enable_if_t<std::is_same_v<typename Field::tag, FreePropertyStat::field::delta>, int> = 0>
void contract_set(const Field&, FreePropertyStat& stat, Value&& value) {
    stat.high = stat.low + static_cast<std::uint64_t>(std::forward<Value>(value));
}

struct FreeTagPropertyStat {
    std::uint64_t low = 30;
    std::uint64_t high = 60;

    CONTRACT(FreeTagPropertyStat,
        PROPERTY(delta, 5, std::uint64_t),
        (low, 6),
        (high, 7))
};

inline std::uint64_t contract_get(
    contract::tag<FreeTagPropertyStat::field::delta>,
    const FreeTagPropertyStat& stat) {
    return stat.high - stat.low;
}

template<class Value>
inline void contract_set(
    contract::tag<FreeTagPropertyStat::field::delta>,
    FreeTagPropertyStat& stat,
    Value&& value) {
    stat.high = stat.low + static_cast<std::uint64_t>(std::forward<Value>(value));
}

struct ReferenceMetric {
    std::uint64_t storage = 0;
    std::uint64_t& raw_count;

    ReferenceMetric()
        : raw_count(storage) {}

    CONTRACT(ReferenceMetric, (raw_count, 1))
};

inline std::uint64_t contract_get(contract::tag<ReferenceMetric::field::raw_count>, const ReferenceMetric& metric) {
    return metric.raw_count * 10;
}

template<class Value>
inline void contract_set(contract::tag<ReferenceMetric::field::raw_count>, ReferenceMetric& metric, Value&& value) {
    metric.raw_count = static_cast<std::uint64_t>(std::forward<Value>(value)) / 10;
}

} // namespace contract_tests
