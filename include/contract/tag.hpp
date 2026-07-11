#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

namespace contract {

template<class T>
struct tag {
    using type = T;
};

} // namespace contract
