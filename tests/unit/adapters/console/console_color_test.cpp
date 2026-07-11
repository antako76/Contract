// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/console/all.hpp>
#include <contract/check.hpp>
#include <contract/contract.hpp>
#include <contract/security.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace {

struct ColorBase {
    std::string title = "alpha";

    CONTRACT(ColorBase, (title, 1, contract::check::max_length(200)))
};

struct ColorRecord : public ColorBase {
    std::vector<std::string> tags{"one", "two"};
    std::string secret = "hidden";

    CONTRACT(ColorRecord,
        BASE(ColorBase, 20),
        (tags, 2),
        (secret, 3, contract::security::no_log()))
};

contract::adapters::console::options colored_options() {
    contract::adapters::console::options opt;
    opt.color.enabled = true;
    opt.field_comment.show_base_offset = true;
    opt.color.palette.field_name = "<F>";
    opt.color.palette.type_name = "<T>";
    opt.color.palette.string_value = "<S>";
    opt.color.palette.comment_field_id = "<I>";
    opt.color.palette.comment_provenance = "<P>";
    opt.color.palette.comment_attribute = "<A>";
    opt.color.palette.comment_security = "<R>";
    opt.color.palette.number_value = "<N>";
    opt.color.palette.comment = "<C>";
    opt.color.palette.truncation = "<X>";
    opt.color.palette.reset = "</>";
    return opt;
}

} // namespace

int main() {
    ColorRecord record;
    const auto output = contract::adapters::console::to_string(record, colored_options());

    assert(output.find("<I>#</><I>21</>") != std::string::npos);
    assert(output.find("<C>std::string</>") != std::string::npos);
    assert(output.find("<P>ColorBase</><C>+</><C>20</>") != std::string::npos);
    assert(output.find("<A>contract::check::max_length(200)</>") != std::string::npos);
    assert(output.find("<C>, </><C>size</><C>=</><C>2</>") != std::string::npos);
    assert(output.find("<R><redacted></>") != std::string::npos);
    assert(output.find("<A>contract::security::no_log()</>") != std::string::npos);

    return 0;
}
