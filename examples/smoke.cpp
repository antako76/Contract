// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/adapters/console.hpp>

#include <iostream>

struct BaseCounters {
    unsigned long long count = 10;
    unsigned long long error = 2;
    unsigned long long time = 30;
    unsigned long long clock = 40;

    CONTRACT(BaseCounters, (count, 1), (error, 2), (time, 3), (clock, 4))
};

int main() {
    BaseCounters counters;

    contract::adapters::console::options options;
    options.output_mode = contract::adapters::console::options::mode::value;

    std::cout << contract::adapters::console::to_string(counters, options);

    return 0;
}
