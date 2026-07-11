#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

namespace contract::adapters::debug {

struct field_comment_options {
    bool show_field_ids = true;
    bool show_field_types = true;
    bool show_storage_type_when_different = true;
    bool show_accessor_kind = true;
    bool show_base_offset = false;
    bool show_attributes = true;
};

} // namespace contract::adapters::debug
