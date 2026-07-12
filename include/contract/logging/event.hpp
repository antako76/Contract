#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/contract.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

namespace contract::logging {

enum class severity {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
};

constexpr std::uint16_t to_number(severity value) noexcept {
    switch (value) {
    case severity::trace:
        return 1;
    case severity::debug:
        return 5;
    case severity::info:
        return 9;
    case severity::warning:
        return 13;
    case severity::error:
        return 17;
    case severity::critical:
        return 21;
    }

    return 0;
}

static std::string_view to_string(severity value) noexcept {
    switch (value) {
    case severity::trace:
        return "trace";
    case severity::debug:
        return "debug";
    case severity::info:
        return "info";
    case severity::warning:
        return "warning";
    case severity::error:
        return "error";
    case severity::critical:
        return "critical";
    }

    return "unknown";
}

template<class T>
struct payload_field {
    using log_payload_field = payload_field<T>;

    std::string_view name;
    const T& value;

    CONTRACT(log_payload_field, 
        (name, 1), 
        REFERENCE(value, 2)
    )
};

template<class T>
payload_field<contract::adapters::base::clean_t<T>> attribute(
    std::string_view name,
    const T& value)
{
    return {name, value};
}

template<class T>
payload_field<contract::adapters::base::clean_t<T>> attr(
    std::string_view name,
    const T& value)
{
    return attribute(name, value);
}

struct empty_context {
    CONTRACT(empty_context)
};

template<class Context, class... Fields>
struct event_view {
    using log_event = event_view<Context, Fields...>;

    std::string timestamp;
    std::string severity;
    std::uint16_t severity_number;
    std::string_view name;
    std::string body;
    const Context& context;
    std::tuple<Fields...> attributes;

    CONTRACT(log_event, 
        (timestamp, 1), 
        (severity, 2), 
        (severity_number, 3), 
        (name, 4), 
        (body, 5), 
        REFERENCE(context, 6),
        (attributes, 7)
    )
};

} // namespace contract::logging
