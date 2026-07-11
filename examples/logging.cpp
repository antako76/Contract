// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/logging.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::string fixed_timestamp() {
    return "2026-06-14T12:34:56.789Z";
}

struct RequestContext {
    std::string_view service;
    std::uint64_t request_id;

    CONTRACT(RequestContext, (service, 1), (request_id, 2))
};

struct Payment {
    std::uint64_t order_id;
    std::string_view currency;

    CONTRACT(Payment, (order_id, 1), (currency, 2))
};

struct SecretPayment {
    std::uint64_t order_id;
    std::string_view token;

    CONTRACT(SecretPayment,
        (order_id, 1),
        (token, 2, contract::security::secret()))
};

} // namespace

int main() {
    using contract::logging::attribute;

    std::ostringstream out;
    contract::logging::options opt{};
    opt.timestamp = fixed_timestamp;

    contract::logging::logger log{out, opt};

    log.info("service_started");

    RequestContext context{};
    context.service = "billing";
    context.request_id = 42;

    Payment payment{};
    payment.order_id = 17;
    payment.currency = "USD";

    SecretPayment secret_payment{};
    secret_payment.order_id = 18;
    secret_payment.token = "tok_live_123";

    log.with(context).info(
        "payment_completed",
        contract::logging::format("Captured payment for order {} in {} ms", payment.order_id, 15),
        attribute("payment", payment),
        attribute("duration_ms", std::uint32_t{15})
    );

    log.with(context).error(
        "payment_failed",
        "Declined by payment gateway",
        attribute("payment", payment),
        attribute("reason", std::string("declined"))
    );

    contract::logging::options redacted_opt = opt;
    redacted_opt.json.secret = contract::adapters::json::security_mode::redact;
    contract::logging::logger redacted_log{out, redacted_opt};

    redacted_log.with(context).info(
        "payment_sensitive",
        "Captured sensitive payment metadata",
        attribute("payment", secret_payment));

    std::cout << out.str();
    return 0;
}
