#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace contract::logging::detail {

inline std::string utc_timestamp() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto milliseconds_since_epoch =
        duration_cast<milliseconds>(now.time_since_epoch());
    const auto seconds_since_epoch =
        duration_cast<seconds>(milliseconds_since_epoch);
    const auto milliseconds_part =
        static_cast<long long>((milliseconds_since_epoch - seconds_since_epoch).count());
    const std::time_t time = system_clock::to_time_t(now);

    std::tm utc{};
#if defined(_WIN32)
    if (gmtime_s(&utc, &time) != 0) {
        return {};
    }
#else
    if (gmtime_r(&time, &utc) == nullptr) {
        return {};
    }
#endif

    char buffer[32]{};
    const int size = std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02dT%02d:%02d:%02d.%03lldZ",
        utc.tm_year + 1900,
        utc.tm_mon + 1,
        utc.tm_mday,
        utc.tm_hour,
        utc.tm_min,
        utc.tm_sec,
        milliseconds_part);

    if (size <= 0 || static_cast<std::size_t>(size) >= sizeof(buffer)) {
        return {};
    }

    return std::string(buffer, static_cast<std::size_t>(size));
}

inline void append_body(std::ostringstream& out, std::string_view text) {
    out << text;
}

template<class T>
void append_body(std::ostringstream& out, T&& value) {
    out << std::forward<T>(value);
}

inline void format_body_impl(std::ostringstream& out, std::string_view fmt) {
    out << fmt;
}

template<class Arg, class... Args>
void format_body_impl(
    std::ostringstream& out,
    std::string_view fmt,
    Arg&& arg,
    Args&&... args)
{
    const auto pos = fmt.find("{}");
    if (pos == std::string_view::npos) {
        out << fmt;
        return;
    }

    out << fmt.substr(0, pos);
    append_body(out, std::forward<Arg>(arg));
    format_body_impl(out, fmt.substr(pos + 2), std::forward<Args>(args)...);
}

template<class... Args>
std::string format(std::string_view fmt, Args&&... args) {
    std::ostringstream out;
    format_body_impl(out, fmt, std::forward<Args>(args)...);
    return out.str();
}

} // namespace contract::logging::detail

namespace contract::logging {

using detail::format;

} // namespace contract::logging
