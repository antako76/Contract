#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/json.hpp>
#include <contract/adapters/json/tuple.hpp>
#include <contract/logging/detail.hpp>
#include <contract/logging/event.hpp>

#include <exception>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace contract::logging {

namespace detail {

template<class T>
struct is_payload_field : std::false_type {};

template<class T>
struct is_payload_field<payload_field<T>> : std::true_type {};

template<class T>
inline constexpr bool is_payload_field_v =
    is_payload_field<std::remove_cvref_t<T>>::value;

template<class T>
inline constexpr bool is_body_arg_v =
    std::is_convertible_v<T, std::string_view> && !is_payload_field_v<T>;

} // namespace detail

using timestamp_provider = std::string (*)();
using error_handler = void (*)(std::exception_ptr) noexcept;

struct options {
    contract::adapters::json::options json{};
    timestamp_provider timestamp = nullptr;
    error_handler on_error = nullptr;

    options() {
        json.no_log = contract::adapters::json::security_mode::omit;
        json.secret = contract::adapters::json::security_mode::omit;
        json.sensitive = contract::adapters::json::security_mode::omit;
    }
};

template<class Logger, class Context>
class context_logger;

template<class Output = std::ostream&>
class logger {
public:
    explicit logger(Output output, options opt = {})
        : out_(output)
        , opt_(std::move(opt)) {}

    template<class Context>
    context_logger<logger, Context> with(Context& context) {
        return {*this, context};
    }

    template<class... Args>
    void trace(std::string_view name, Args&&... args) noexcept {
        write(severity::trace, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void debug(std::string_view name, Args&&... args) noexcept {
        write(severity::debug, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void info(std::string_view name, Args&&... args) noexcept {
        write(severity::info, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void warning(std::string_view name, Args&&... args) noexcept {
        write(severity::warning, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void error(std::string_view name, Args&&... args) noexcept {
        write(severity::error, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void critical(std::string_view name, Args&&... args) noexcept {
        write(severity::critical, name, std::forward<Args>(args)...);
    }

private:
    template<class, class>
    friend class context_logger;

    template<class... Args>
    void write(severity level, std::string_view name, Args&&... args) noexcept
    {
        const empty_context context{};
        write_with_context(level, name, context, std::forward<Args>(args)...);
    }

    template<class Context, class... Fields>
    void write_with_context(
        severity level,
        std::string_view name,
        const Context& context,
        Fields&&... fields) noexcept
    {
        try {
            write_event(level, name, context, std::forward<Fields>(fields)...);
            out_ << '\n';
        } catch (...) {
            report_error(std::current_exception());
        }
    }

    template<class Context>
    void write_event(
        severity level,
        std::string_view name,
        const Context& context)
    {
        write_event(level, name, std::string_view{}, context);
    }

    template<class Context, class First, class... Rest>
    void write_event(
        severity level,
        std::string_view name,
        const Context& context,
        First&& first,
        Rest&&... rest)
    {
        if constexpr (detail::is_body_arg_v<First>) {
            write_event(
                level,
                name,
                std::string_view(first),
                context,
                std::forward<Rest>(rest)...);
        } else {
            write_event(
                level,
                name,
                std::string_view{},
                context,
                std::forward<First>(first),
                std::forward<Rest>(rest)...);
        }
    }

    template<class Context, class... Fields>
    void write_event(
        severity level,
        std::string_view name,
        std::string_view body,
        const Context& context,
        Fields&&... fields)
    {
        using event_type = event_view<
            Context,
            contract::adapters::base::clean_t<Fields>...>;

        event_type event{
            opt_.timestamp ? opt_.timestamp() : detail::utc_timestamp(),
            std::string(to_string(level)),
            to_number(level),
            name,
            std::string(body),
            context,
            std::tuple<contract::adapters::base::clean_t<Fields>...>{
                std::forward<Fields>(fields)...},
        };

        contract::adapters::json::writer<Output>{out_, opt_.json} << event;
    }

    void report_error(std::exception_ptr error) noexcept {
        if (opt_.on_error == nullptr) {
            return;
        }

        try {
            opt_.on_error(std::move(error));
        } catch (...) {
        }
    }

    Output out_;
    options opt_{};
};

template<class Output>
logger(Output&) -> logger<Output&>;

template<class Output>
logger(Output&, options) -> logger<Output&>;

template<class Logger, class Context>
class context_logger {
public:
    context_logger(Logger& owner, const Context& context)
        : owner_(owner)
        , context_(context) {}

    template<class... Args>
    void trace(std::string_view name, Args&&... args) const noexcept {
        write(severity::trace, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void debug(std::string_view name, Args&&... args) const noexcept {
        write(severity::debug, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void info(std::string_view name, Args&&... args) const noexcept {
        write(severity::info, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void warning(std::string_view name, Args&&... args) const noexcept {
        write(severity::warning, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void error(std::string_view name, Args&&... args) const noexcept {
        write(severity::error, name, std::forward<Args>(args)...);
    }

    template<class... Args>
    void critical(std::string_view name, Args&&... args) const noexcept {
        write(severity::critical, name, std::forward<Args>(args)...);
    }

private:
    Logger& owner_;
    const Context& context_;
    template<class... Args>
    void write(severity level, std::string_view name, Args&&... args) const noexcept
    {
        owner_.write_with_context(level, name, context_, std::forward<Args>(args)...);
    }
};

} // namespace contract::logging
