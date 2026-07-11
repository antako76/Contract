#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace contract {

struct attr_targets {
    bool type = false;
    bool field = false;
    bool enum_type = false;
    bool enum_value = false;
    bool overlay = false;
};

template<class Attr>
struct attr_traits;

template<class Attr>
struct attribute_descriptor {
    using attribute_type = Attr;

    Attr attribute;
    std::string_view source{};
};

namespace detail {

template<class T>
using clean_t = std::remove_cvref_t<T>;

template<class Attr, class Tuple, std::size_t Index = 0>
constexpr auto find_attribute(Tuple& entries) {
    if constexpr (Index == std::tuple_size_v<clean_t<Tuple>>) {
        return static_cast<Attr*>(nullptr);
    } else {
        using descriptor_type = clean_t<std::tuple_element_t<Index, clean_t<Tuple>>>;
        using current_attr = typename descriptor_type::attribute_type;

        if constexpr (std::is_same_v<current_attr, clean_t<Attr>>) {
            return &std::get<Index>(entries).attribute;
        } else {
            return find_attribute<Attr, Tuple, Index + 1>(entries);
        }
    }
}

} // namespace detail

template<class... Attrs>
struct attributes {
    using tuple_type = std::tuple<attribute_descriptor<Attrs>...>;
    [[no_unique_address]]
    tuple_type entries;

    template<class Attr>
    static constexpr bool contains() {
        return (std::is_same_v<detail::clean_t<Attr>, Attrs> || ...);
    }

    template<class Attr>
    constexpr auto find() & {
        return detail::find_attribute<detail::clean_t<Attr>>(entries);
    }

    template<class Attr>
    constexpr auto find() const & {
        return detail::find_attribute<const detail::clean_t<Attr>>(entries);
    }

    template<class Attr>
    constexpr auto find() && = delete;

    template<class Attr>
    constexpr decltype(auto) get() & {
        static_assert(contains<Attr>(), "CONTRACT attribute is not present");
        return *find<Attr>();
    }

    template<class Attr>
    constexpr decltype(auto) get() const & {
        static_assert(contains<Attr>(), "CONTRACT attribute is not present");
        return *find<Attr>();
    }
};

namespace detail {

template<class Attr>
concept has_attr_traits = requires {
    typename attr_traits<Attr>::vocabulary;
    attr_traits<Attr>::targets;
    attr_traits<Attr>::repeatable;
};

template<class... Attrs>
inline constexpr bool has_all_attr_traits_v = (has_attr_traits<Attrs> && ...);

template<class... Attrs>
struct valid_attribute_pack : std::true_type {};

template<class First, class... Rest>
struct valid_attribute_pack<First, Rest...>
    : std::bool_constant<
          ((!std::is_same_v<First, Rest> || attr_traits<First>::repeatable) && ...) &&
          valid_attribute_pack<Rest...>::value> {};

template<class... Attrs>
constexpr void check_attribute_pack() {
    static_assert(has_all_attr_traits_v<Attrs...>,
        "CONTRACT attributes require contract::attr_traits<Attr>");

    if constexpr (has_all_attr_traits_v<Attrs...>) {
        static_assert(valid_attribute_pack<Attrs...>::value,
            "CONTRACT non-repeatable attributes must not be duplicated");
    }
}

template<class... Attrs>
constexpr void check_contract_attributes() {
    check_attribute_pack<Attrs...>();

    if constexpr (has_all_attr_traits_v<Attrs...>) {
        static_assert((attr_traits<Attrs>::targets.type && ...),
            "CONTRACT ATTRS(...) contains an attribute that does not target contracts");
    }
}

template<class... Attrs>
constexpr void check_field_attributes() {
    check_attribute_pack<Attrs...>();

    if constexpr (has_all_attr_traits_v<Attrs...>) {
        static_assert((attr_traits<Attrs>::targets.field && ...),
            "CONTRACT field contains an attribute that does not target fields");
    }
}

template<class AttributeSet, class Attr>
struct attribute_count_in;

template<class... Attrs, class Attr>
struct attribute_count_in<attributes<Attrs...>, Attr>
    : std::integral_constant<std::size_t,
          (std::size_t{0} + ... + (std::is_same_v<Attrs, Attr> ? 1u : 0u))> {};

template<class AttributeSet, class Vocabulary>
struct attributes_in_vocabulary_impl;

template<class Vocabulary>
struct attributes_in_vocabulary_impl<attributes<>, Vocabulary> {
    using type = attributes<>;
};

template<class First, class... Rest, class Vocabulary>
struct attributes_in_vocabulary_impl<attributes<First, Rest...>, Vocabulary> {
private:
    using tail = typename attributes_in_vocabulary_impl<attributes<Rest...>, Vocabulary>::type;

    template<class Head, class Tail>
    struct prepend;

    template<class Head, class... Tail>
    struct prepend<Head, attributes<Tail...>> {
        using type = attributes<Head, Tail...>;
    };

public:
    using type = std::conditional_t<
        std::is_same_v<typename attr_traits<First>::vocabulary, Vocabulary>,
        typename prepend<First, tail>::type,
        tail>;
};

template<class Vocabulary, class Descriptor>
constexpr auto attribute_in_vocabulary(const Descriptor& descriptor) {
    using attr_type = typename clean_t<Descriptor>::attribute_type;

    if constexpr (std::is_same_v<typename attr_traits<attr_type>::vocabulary, Vocabulary>) {
        return std::tuple<Descriptor>{descriptor};
    } else {
        return std::tuple<>{};
    }
}

} // namespace detail

template<class Attr>
constexpr auto describe_attribute(Attr&& attr, std::string_view source) {
    return attribute_descriptor<std::decay_t<Attr>>{
        std::forward<Attr>(attr),
        source
    };
}

template<class Descriptor>
struct attributes_of {
    using type = typename detail::clean_t<Descriptor>::attributes_type;
};

template<class Descriptor>
using attributes_of_t = typename attributes_of<Descriptor>::type;

template<class Descriptor, class Attr>
struct attribute_count
    : detail::attribute_count_in<
          attributes_of_t<Descriptor>,
          detail::clean_t<Attr>> {};

template<class Descriptor, class Attr>
inline constexpr std::size_t attribute_count_v = attribute_count<Descriptor, Attr>::value;

template<class Descriptor, class Attr>
struct has_attribute : std::bool_constant<(attribute_count_v<Descriptor, Attr> != 0)> {};

template<class Descriptor, class Attr>
inline constexpr bool has_attribute_v = has_attribute<Descriptor, Attr>::value;

template<class Descriptor, class Vocabulary>
struct attributes_in_vocabulary {
    using type = typename detail::attributes_in_vocabulary_impl<
        attributes_of_t<Descriptor>,
        Vocabulary>::type;
};

template<class Descriptor, class Vocabulary>
using attributes_in_vocabulary_t =
    typename attributes_in_vocabulary<Descriptor, Vocabulary>::type;

template<class Descriptor>
constexpr auto& get_attributes(Descriptor& descriptor) {
    return descriptor.attributes;
}

template<class Descriptor>
constexpr const auto& get_attributes(const Descriptor& descriptor) {
    return descriptor.attributes;
}

template<class Descriptor>
    requires (!std::is_lvalue_reference_v<Descriptor>)
constexpr void get_attributes(Descriptor&&) = delete;

template<class... Attrs>
constexpr auto make_attributes(attribute_descriptor<Attrs>... attrs) {
    using result_type = attributes<Attrs...>;
    return result_type{
        typename result_type::tuple_type{std::move(attrs)...}
    };
}

template<class... Attrs>
constexpr auto make_contract_attributes(attribute_descriptor<Attrs>... attrs) {
    detail::check_contract_attributes<Attrs...>();
    return make_attributes(std::move(attrs)...);
}

template<class... Attrs>
constexpr auto make_field_attributes(attribute_descriptor<Attrs>... attrs) {
    detail::check_field_attributes<Attrs...>();
    return make_attributes(std::move(attrs)...);
}

template<class Vocabulary, class... Attrs>
constexpr auto get_attributes_in_vocabulary(const attributes<Attrs...>& attrs) {
    using result_type =
        typename detail::attributes_in_vocabulary_impl<attributes<Attrs...>, Vocabulary>::type;

    return result_type{
        std::apply(
            [](const auto&... entries) {
                return std::tuple_cat(
                    detail::attribute_in_vocabulary<Vocabulary>(entries)...);
            },
            attrs.entries)
    };
}

template<class Vocabulary, class Descriptor>
constexpr auto get_attributes_in_vocabulary(const Descriptor& descriptor) {
    return get_attributes_in_vocabulary<Vocabulary>(descriptor.attributes);
}

template<class... Left, class... Right>
constexpr auto compose_attributes(
    const attributes<Left...>& left,
    const attributes<Right...>& right) {
    detail::check_attribute_pack<Left..., Right...>();

    using result_type = attributes<Left..., Right...>;
    return result_type{std::tuple_cat(left.entries, right.entries)};
}

} // namespace contract
