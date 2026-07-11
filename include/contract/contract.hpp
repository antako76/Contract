#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

// Public core header for the CONTRACT layer.
//
// Keep this file small: it should include the base model and DSL macros only.
// Policy and adapter-specific headers belong in their own opt-in includes.

#include <contract/macros.hpp>
#include <contract/attributes/attributes.hpp>
#include <contract/attributes/adapter_traits.hpp>
#include <contract/definition.hpp>
#include <contract/field.hpp>
#include <contract/tag.hpp>
#include <contract/visit.hpp>
