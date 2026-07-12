#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/attributes.hpp>
#include <contract/detail/detection.hpp>

#include <string_view>
#include <type_traits>
#include <utility>

namespace contract {

enum class field_kind {
    member,
    reference,
    property,
};

template<class T>
struct member_pointer_traits;

template<class Owner, class Value>
struct member_pointer_traits<Value Owner::*> {
    using storage_type = Value;
    using value_type = std::remove_cvref_t<Value>;
};

namespace detail {

template<class Value, class T>
constexpr decltype(auto) copy_from_volatile_if_needed(T&& value) {
    if constexpr (std::is_volatile_v<std::remove_reference_t<T>>) {
        return Value(std::forward<T>(value));
    } else {
        return std::forward<T>(value);
    }
}

template<field_kind Kind, class Owner, class DeclaredValue, auto Member, class Identity>
struct field_types {
    using storage_type = void;
    using value_type = DeclaredValue;
};

template<class Owner, class DeclaredValue, auto Member, class Identity>
struct field_types<field_kind::member, Owner, DeclaredValue, Member, Identity> {
    using traits = member_pointer_traits<decltype(Member)>;
    using storage_type = typename traits::storage_type;
    using value_type = typename traits::value_type;
};

template<class Owner, class DeclaredValue, auto Member, class Identity>
struct field_types<field_kind::reference, Owner, DeclaredValue, Member, Identity> {
    using storage_type = DeclaredValue;
    using value_type = std::remove_cvref_t<storage_type>;
};

} // namespace detail

// The adapter-facing descriptor. Member access is represented directly by a
// member pointer. REFERENCE descriptors pass their own type as Identity so the
// only generated fallback code lives on that descriptor, not on a separate tag.
template<
    class Owner,
    int DeclaredId,
    field_kind Kind,
    class Attributes = attributes<>,
    class DeclaredValue = void,
    auto Member = nullptr,
    class Identity = void>
struct field {
    using self_type = field;
    using identity_type = std::conditional_t<std::is_void_v<Identity>, self_type, Identity>;
    using owner_type = Owner;
    using attributes_type = Attributes;

    static constexpr field_kind kind = Kind;
    static constexpr bool is_base_import = false;
    static constexpr int declared_id = DeclaredId;
    static constexpr int id = DeclaredId;
    static constexpr int base_offset = 0;

    static_assert(kind != field_kind::property || !std::is_void_v<DeclaredValue>,
        "PROPERTY fields must carry their declared value type");
    static_assert(kind != field_kind::reference || !std::is_void_v<DeclaredValue>,
        "REFERENCE fields must carry their declared value type");
    static_assert(kind != field_kind::member || std::is_member_object_pointer_v<decltype(Member)>,
        "member fields require a member pointer");

    using storage_type = typename detail::field_types<kind, Owner, DeclaredValue, Member, identity_type>::storage_type;
    using value_type = typename detail::field_types<kind, Owner, DeclaredValue, Member, identity_type>::value_type;

    std::string_view name;
    [[no_unique_address]] attributes_type attributes;

    constexpr field(std::string_view field_name, attributes_type field_attributes)
        : name(field_name), attributes(std::move(field_attributes)) {}

    template<class Object>
    static constexpr bool has_custom_get =
        detail::has_free_contract_get<identity_type, Object> ||
        detail::has_member_contract_get<Object, identity_type>;

    template<class Object, class Value>
    static constexpr bool has_custom_set =
        detail::has_free_contract_set<identity_type, Object, Value> ||
        detail::has_member_contract_set<Object, identity_type, Value>;

    template<class Object>
    static constexpr bool can_direct_ref = [] {
        if constexpr (kind == field_kind::member) {
            return detail::can_member_ref<Object, Member>;
        } else if constexpr (kind == field_kind::reference) {
            return std::is_reference_v<decltype(identity_type::contract_ref_get(std::declval<Object&>()))>;
        } else {
            return false;
        }
    }();

    template<class Object>
    constexpr decltype(auto) ref(Object& obj) const
        requires (kind != field_kind::property)
    {
        if constexpr (kind == field_kind::member) {
            return obj.*Member;
        } else {
            return identity_type::contract_ref_get(obj);
        }
    }

    template<class Object>
    constexpr decltype(auto) ref(const Object& obj) const
        requires (kind != field_kind::property)
    {
        if constexpr (kind == field_kind::member) {
            return obj.*Member;
        } else {
            return identity_type::contract_ref_get(obj);
        }
    }

    template<class Object>
    constexpr decltype(auto) ref(Object&&) const = delete;

    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        const auto& identity = static_cast<const identity_type&>(*this);
        if constexpr (detail::has_free_contract_get<identity_type, Object&>) {
            return contract_get(identity, obj);
        } else if constexpr (detail::has_member_contract_get<Object, identity_type>) {
            return obj.contract_get(identity);
        } else if constexpr (kind != field_kind::property) {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field, object) or object.contract_get(field)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        const auto& identity = static_cast<const identity_type&>(*this);
        if constexpr (detail::has_free_contract_get<identity_type, const Object&>) {
            return contract_get(identity, obj);
        } else if constexpr (detail::has_member_contract_get<const Object, identity_type>) {
            return obj.contract_get(identity);
        } else if constexpr (kind != field_kind::property) {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field, object) or object.contract_get(field)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object& obj, Value&& value) const {
        const auto& identity = static_cast<const identity_type&>(*this);
        if constexpr (detail::has_free_contract_set<identity_type, Object&, Value&&>) {
            contract_set(identity, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_member_contract_set<Object, identity_type, Value&&>) {
            obj.contract_set(identity, std::forward<Value>(value));
        } else if constexpr (kind == field_kind::member) {
            ref(obj) = std::forward<Value>(value);
        } else if constexpr (kind == field_kind::reference) {
            identity_type::contract_ref_set(obj, std::forward<Value>(value));
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_set(field, object, value) or object.contract_set(field, value)");
        }
    }

    template<class Object, class Value>
    constexpr void set(const Object&, Value&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object&&, Value&&) const = delete;

    template<int Offset>
    constexpr auto imported() const;
};

template<class DeclaredField, int EffectiveId>
struct imported_field : DeclaredField {
    using declared_field = DeclaredField;
    using DeclaredField::DeclaredField;

    static constexpr bool is_base_import = true;
    static constexpr int id = EffectiveId;
    static constexpr int base_offset = EffectiveId - DeclaredField::declared_id;

    constexpr imported_field(const DeclaredField& value)
        : DeclaredField(value) {}

    template<int Offset>
    constexpr auto imported() const {
        return imported_field<DeclaredField, EffectiveId + Offset>{
            static_cast<const DeclaredField&>(*this)};
    }
};

template<class Owner, int Id, auto Member, class... Attrs>
constexpr auto make_member_field(std::string_view name, attributes<Attrs...> field_attributes) {
    return field<Owner, Id, field_kind::member, attributes<Attrs...>, void, Member>{
        name, std::move(field_attributes)};
}

template<class Owner, int Id, class Value, class... Attrs>
constexpr auto make_property_field(std::string_view name, attributes<Attrs...> field_attributes) {
    return field<Owner, Id, field_kind::property, attributes<Attrs...>, Value>{
        name, std::move(field_attributes)};
}

template<
    class Owner,
    int DeclaredId,
    field_kind Kind,
    class Attributes,
    class DeclaredValue,
    auto Member,
    class Identity>
template<int Offset>
constexpr auto field<Owner, DeclaredId, Kind, Attributes, DeclaredValue, Member, Identity>::imported() const {
    return imported_field<field, DeclaredId + Offset>{*this};
}

// A pure compile-time marker: flatten_entry() matches Base/Offset by template
// deduction, so no members are needed here.
template<class Base, int Offset>
struct base {};

} // namespace contract
