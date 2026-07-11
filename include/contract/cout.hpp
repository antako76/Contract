#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/io/cout.hpp>

namespace contract {

using cout_stream = io::cout_stream;
inline cout_stream cout = io::cout.schema();

} // namespace contract
