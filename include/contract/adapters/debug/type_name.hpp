#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/adapters/base.hpp>
#include <contract/definition.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <bitset>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

namespace contract::adapters::debug {

template<class T>
std::string type_name();

namespace detail {

template<class... Ts>
std::string joined_type_names() {
    std::string result;
    bool first = true;

    ((result += (first ? "" : ","), result += type_name<Ts>(), first = false), ...);

    return result;
}

} // namespace detail

template<class T>
std::string scalar_type_name() {
    using value_type = contract::adapters::base::clean_t<T>;

    if constexpr (std::is_same_v<value_type, bool>) {
        return "bool";
    } else if constexpr (std::is_same_v<value_type, char>) {
        return "char";
    } else if constexpr (std::is_same_v<value_type, signed char> ||
                         std::is_same_v<value_type, std::int8_t>) {
        return "i8";
    } else if constexpr (std::is_same_v<value_type, unsigned char> ||
                         std::is_same_v<value_type, std::uint8_t>) {
        return "u8";
    } else if constexpr (std::is_same_v<value_type, short> ||
                         std::is_same_v<value_type, std::int16_t>) {
        return "i16";
    } else if constexpr (std::is_same_v<value_type, unsigned short> ||
                         std::is_same_v<value_type, std::uint16_t>) {
        return "u16";
    } else if constexpr (std::is_same_v<value_type, int> ||
                         std::is_same_v<value_type, std::int32_t>) {
        return "i32";
    } else if constexpr (std::is_same_v<value_type, unsigned int> ||
                         std::is_same_v<value_type, std::uint32_t>) {
        return "u32";
    } else if constexpr (std::is_same_v<value_type, long> ||
                         std::is_same_v<value_type, std::int64_t>) {
        return "i64";
    } else if constexpr (std::is_same_v<value_type, unsigned long> ||
                         std::is_same_v<value_type, unsigned long long> ||
                         std::is_same_v<value_type, std::uint64_t>) {
        return "u64";
    } else if constexpr (std::is_same_v<value_type, float>) {
        return "float";
    } else if constexpr (std::is_same_v<value_type, double>) {
        return "double";
    } else if constexpr (std::is_same_v<value_type, long double>) {
        return "long double";
    } else if constexpr (std::is_same_v<value_type, std::byte>) {
        return "std::byte";
    } else {
        return typeid(value_type).name();
    }
}

template<class T>
struct type_name_impl {
    static std::string get() {
        using value_type = contract::adapters::base::clean_t<T>;

        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            return std::string(contract::type_name<value_type>());
        } else if constexpr (std::is_arithmetic_v<value_type> || std::is_enum_v<value_type> ||
                             std::is_same_v<value_type, std::byte>) {
            return scalar_type_name<value_type>();
        } else {
            return typeid(value_type).name();
        }
    }
};

template<>
struct type_name_impl<std::string> {
    static std::string get() {
        return "std::string";
    }
};

template<>
struct type_name_impl<std::string_view> {
    static std::string get() {
        return "std::string_view";
    }
};

template<class T>
struct type_name_impl<std::vector<T>> {
    static std::string get() {
        return "std::vector<" + type_name<T>() + ">";
    }
};

template<class T, std::size_t N>
struct type_name_impl<std::array<T, N>> {
    static std::string get() {
        return "std::array<" + type_name<T>() + "," + std::to_string(N) + ">";
    }
};

template<class T>
struct type_name_impl<std::optional<T>> {
    static std::string get() {
        return "std::optional<" + type_name<T>() + ">";
    }
};

template<class K, class V, class Compare, class Allocator>
struct type_name_impl<std::map<K, V, Compare, Allocator>> {
    static std::string get() {
        return "std::map<" + type_name<K>() + "," + type_name<V>() + ">";
    }
};

template<class K, class V, class Hash, class KeyEqual, class Allocator>
struct type_name_impl<std::unordered_map<K, V, Hash, KeyEqual, Allocator>> {
    static std::string get() {
        return "std::unordered_map<" + type_name<K>() + "," + type_name<V>() + ">";
    }
};

template<class... Ts>
struct type_name_impl<std::tuple<Ts...>> {
    static std::string get() {
        return "std::tuple<" + detail::joined_type_names<Ts...>() + ">";
    }
};

template<class... Ts>
struct type_name_impl<std::variant<Ts...>> {
    static std::string get() {
        return "std::variant<" + detail::joined_type_names<Ts...>() + ">";
    }
};

template<class T, std::size_t N>
struct type_name_impl<T[N]> {
    static std::string get() {
        return type_name<T>() + "[" + std::to_string(N) + "]";
    }
};

template<std::size_t N>
struct type_name_impl<std::bitset<N>> {
    static std::string get() {
        return "std::bitset<" + std::to_string(N) + ">";
    }
};

template<class T>
std::string type_name() {
    using reference_removed_type = std::remove_reference_t<T>;
    using value_type = std::remove_cv_t<reference_removed_type>;

    std::string name = type_name_impl<value_type>::get();

    if constexpr (!contract::adapters::base::has_contract_definition<value_type>) {
        if constexpr (std::is_const_v<reference_removed_type>) {
            name = "const " + name;
        }

        if constexpr (std::is_volatile_v<reference_removed_type>) {
            name = "volatile " + name;
        }
    }

    return name;
}

} // namespace contract::adapters::debug
