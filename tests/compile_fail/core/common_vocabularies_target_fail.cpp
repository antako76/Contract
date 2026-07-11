// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/contract.hpp>
#include <contract/check.hpp>
#include <contract/doc.hpp>
#include <contract/schema.hpp>

struct CommonVocabularyTargetFail {
    CONTRACT(CommonVocabularyTargetFail,
        ATTRS(contract::schema::type(contract::schema::string)))
};
