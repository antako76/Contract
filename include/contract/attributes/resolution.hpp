#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/tag.hpp>
#include <contract/attributes/adapter_traits.hpp>

#include <type_traits>

namespace contract {

enum class attribute_mode {
    enforce,
    hint,
    display,
    ignore,
    out_of_scope,
    reject,
    error
};

enum class attribute_resolution_kind {
    invisible,
    visible
};

struct attribute_resolution {
    attribute_resolution_kind kind = attribute_resolution_kind::invisible;
    attribute_mode mode = attribute_mode::error;

    static constexpr attribute_resolution invisible() {
        return {};
    }

    static constexpr attribute_resolution from_mode(attribute_mode value) {
        return {attribute_resolution_kind::visible, value};
    }
};

template<class... Rules>
struct attribute_rules {};

template<class Vocabulary, attribute_mode Mode>
struct vocabulary_mode_rule {
    using vocabulary = Vocabulary;
    static constexpr attribute_mode mode = Mode;
};

template<class Attr, attribute_mode Mode>
struct exact_mode_rule {
    using attr = detail::clean_t<Attr>;
    static constexpr attribute_mode mode = Mode;
};

template<auto Tag, attribute_mode Mode>
struct tag_mode_rule {
    static constexpr auto tag = Tag;
    static constexpr attribute_mode mode = Mode;
};

template<class Vocabulary>
struct default_for {
    using enforce = vocabulary_mode_rule<Vocabulary, attribute_mode::enforce>;
    using hint = vocabulary_mode_rule<Vocabulary, attribute_mode::hint>;
    using display = vocabulary_mode_rule<Vocabulary, attribute_mode::display>;
    using ignore = vocabulary_mode_rule<Vocabulary, attribute_mode::ignore>;
    using out_of_scope = vocabulary_mode_rule<Vocabulary, attribute_mode::out_of_scope>;
    using reject = vocabulary_mode_rule<Vocabulary, attribute_mode::reject>;
    using error = vocabulary_mode_rule<Vocabulary, attribute_mode::error>;
};

template<class Attr>
struct for_attr {
    using enforce = exact_mode_rule<Attr, attribute_mode::enforce>;
    using hint = exact_mode_rule<Attr, attribute_mode::hint>;
    using display = exact_mode_rule<Attr, attribute_mode::display>;
    using ignore = exact_mode_rule<Attr, attribute_mode::ignore>;
    using out_of_scope = exact_mode_rule<Attr, attribute_mode::out_of_scope>;
    using reject = exact_mode_rule<Attr, attribute_mode::reject>;
    using error = exact_mode_rule<Attr, attribute_mode::error>;
};

template<auto Tag>
struct for_tag {
    using enforce = tag_mode_rule<Tag, attribute_mode::enforce>;
    using hint = tag_mode_rule<Tag, attribute_mode::hint>;
    using display = tag_mode_rule<Tag, attribute_mode::display>;
    using ignore = tag_mode_rule<Tag, attribute_mode::ignore>;
    using out_of_scope = tag_mode_rule<Tag, attribute_mode::out_of_scope>;
    using reject = tag_mode_rule<Tag, attribute_mode::reject>;
    using error = tag_mode_rule<Tag, attribute_mode::error>;
};

namespace detail {

template<class Traits>
concept has_attribute_rules_declaration = requires {
    typename Traits::attribute_rules;
};

template<class Result, class Value>
constexpr void update_rule_result(Result& result, bool matched, int specificity, Value value) {
    if (!matched) {
        return;
    }

    if (!result.matched || specificity > result.specificity) {
        result.matched = true;
        result.conflict = false;
        result.specificity = specificity;
        result.value = value;
        return;
    }

    if (specificity == result.specificity && value != result.value) {
        result.conflict = true;
    }
}

struct rule_result {
    bool matched = false;
    bool conflict = false;
    int specificity = 0;
    attribute_mode value = attribute_mode::error;
};

template<class Rule>
struct rule_specificity : std::integral_constant<int, 0> {};

template<class Vocabulary, attribute_mode Mode>
struct rule_specificity<vocabulary_mode_rule<Vocabulary, Mode>> : std::integral_constant<int, 1> {};

template<class Attr, attribute_mode Mode>
struct rule_specificity<exact_mode_rule<Attr, Mode>> : std::integral_constant<int, 3> {};

template<auto Tag, attribute_mode Mode>
struct rule_specificity<tag_mode_rule<Tag, Mode>> : std::integral_constant<int, 2> {};

template<class Rule, class Attr, class = void>
struct rule_match : std::false_type {
    static constexpr int specificity = 0;
    static constexpr attribute_mode mode = attribute_mode::error;
};

template<class Vocabulary, attribute_mode Mode, class Attr>
struct rule_match<vocabulary_mode_rule<Vocabulary, Mode>, Attr, std::void_t<>>
    : std::bool_constant<
          std::is_same_v<
              typename attr_traits<clean_t<Attr>>::vocabulary,
              Vocabulary>> {
    static constexpr int specificity = 1;
    static constexpr attribute_mode mode = Mode;
};

template<class AttrType, attribute_mode Mode, class Attr>
struct rule_match<exact_mode_rule<AttrType, Mode>, Attr, std::void_t<>>
    : std::bool_constant<std::is_same_v<clean_t<Attr>, AttrType>> {
    static constexpr int specificity = 3;
    static constexpr attribute_mode mode = Mode;
};

template<auto Tag, attribute_mode Mode, class Attr>
struct rule_match<tag_mode_rule<Tag, Mode>, Attr, std::void_t<>>
    : std::bool_constant<has_attribute_tag_v<Attr, Tag>> {
    static constexpr int specificity = 2;
    static constexpr attribute_mode mode = Mode;
};

template<class Attr, class... Rules>
constexpr rule_result resolve_mode_rules(const attribute_rules<Rules...>&) {
    rule_result result{};
    (update_rule_result(
         result,
         rule_match<Rules, Attr>::value,
         rule_match<Rules, Attr>::specificity,
         rule_match<Rules, Attr>::mode), ...);
    return result;
}

template<class Traits, class Attr>
constexpr attribute_mode resolve_visible_attribute_mode() {
    if constexpr (has_attribute_rules_declaration<Traits>) {
        constexpr auto rules =
            resolve_mode_rules<Attr>(typename Traits::attribute_rules{});
        static_assert(!rules.conflict,
            "CONTRACT attribute_rules produce conflicting results for the attribute");

        if (rules.matched) {
            return rules.value;
        } else if constexpr (has_all_attributes_read_only_v<Traits>) {
            return attribute_mode::display;
        } else {
            return attribute_mode::error;
        }
    } else if constexpr (has_all_attributes_read_only_v<Traits>) {
        return attribute_mode::display;
    } else {
        return attribute_mode::error;
    }
}

} // namespace detail

template<class Traits, class Attr>
constexpr attribute_resolution resolve_attribute_mode(const Attr& attr) {
    using attr_type = detail::clean_t<Attr>;

    if constexpr (!is_attribute_visible_v<Traits, attr_type>) {
        return attribute_resolution::invisible();
    } else {
        return attribute_resolution::from_mode(
            detail::resolve_visible_attribute_mode<Traits, attr_type>());
    }
}

} // namespace contract
