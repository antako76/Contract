// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/logging.hpp>

#include <cassert>
#include <cstdint>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string fixed_timestamp() {
    return "2026-06-14T12:34:56.789Z";
}

bool error_reported = false;

void record_error(std::exception_ptr error) noexcept {
    error_reported = error != nullptr;
}

struct RequestContext {
    std::string_view service = "billing";
    std::uint64_t request_id = 42;

    CONTRACT(RequestContext, (service, 1), (request_id, 2))
};

struct Payment {
    std::uint64_t order_id = 17;
    std::string_view currency = "USD";

    CONTRACT(Payment, (order_id, 1), (currency, 2))
};

struct SecretPayment {
    std::uint64_t order_id = 18;
    std::string_view token = "tok_live_123";

    CONTRACT(SecretPayment,
        (order_id, 1),
        (token, 2, contract::security::secret()))
};

struct throwing_output {
    template<class T>
    throwing_output& operator<<(const T&) {
        throw std::runtime_error("logging output failed");
    }
};

} // namespace

int main() {
    using contract::logging::attribute;

    contract::logging::options opt{};
    opt.timestamp = fixed_timestamp;

    std::ostringstream plain_output;
    contract::logging::logger plain_log{plain_output, opt};
    plain_log.info("service_started", attribute("port", std::uint32_t{8080}));

    assert(plain_output.str() ==
        "{\"timestamp\":\"2026-06-14T12:34:56.789Z\","
        "\"severity\":\"info\","
        "\"severity_number\":9,"
        "\"name\":\"service_started\","
        "\"body\":\"\","
        "\"context\":{},"
        "\"attributes\":[{\"name\":\"port\",\"value\":8080}]}\n");

    std::ostringstream contextual_output;
    contract::logging::logger contextual_log{contextual_output, opt};
    const RequestContext context{};
    const Payment payment{};

    contextual_log.with(context).error(
        "payment_failed",
        contract::logging::format(
            "Declined by payment gateway for order {}",
            payment.order_id),
        attribute("payment", payment),
        attribute("reason", std::string("declined")));

    assert(contextual_output.str() ==
        "{\"timestamp\":\"2026-06-14T12:34:56.789Z\","
        "\"severity\":\"error\","
        "\"severity_number\":17,"
        "\"name\":\"payment_failed\","
        "\"body\":\"Declined by payment gateway for order 17\","
        "\"context\":{\"service\":\"billing\",\"request_id\":42},"
        "\"attributes\":["
            "{\"name\":\"payment\",\"value\":{\"order_id\":17,\"currency\":\"USD\"}},"
            "{\"name\":\"reason\",\"value\":\"declined\"}"
        "]}\n");

    std::ostringstream redacted_output;
    contract::logging::options redacted_opt = opt;
    redacted_opt.json.secret = contract::adapters::json::security_mode::redact;
    contract::logging::logger redacted_log{redacted_output, redacted_opt};
    const SecretPayment secret_payment{};

    redacted_log.with(context).info(
        "payment_sensitive",
        "Captured sensitive payment metadata",
        attribute("payment", secret_payment));

    assert(redacted_output.str() ==
        "{\"timestamp\":\"2026-06-14T12:34:56.789Z\","
        "\"severity\":\"info\","
        "\"severity_number\":9,"
        "\"name\":\"payment_sensitive\","
        "\"body\":\"Captured sensitive payment metadata\","
        "\"context\":{\"service\":\"billing\",\"request_id\":42},"
        "\"attributes\":["
            "{\"name\":\"payment\",\"value\":{\"order_id\":18,\"token\":\"<redacted>\"}}"
        "]}\n");

    throwing_output failed_output;
    contract::logging::options failed_opt{};
    failed_opt.timestamp = fixed_timestamp;
    failed_opt.on_error = record_error;
    contract::logging::logger failed_log{failed_output, failed_opt};
    failed_log.critical("output_failed");
    assert(error_reported);

    return 0;
}
