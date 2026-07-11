#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>

#include <iostream>

namespace contract::io {

using cout_stream = contract::adapters::console::writer<std::ostream&>;

inline cout_stream cout{std::cout};

} // namespace contract::io
