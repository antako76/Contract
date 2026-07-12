#pragma once

// Copyright 2026 Ilya Korolev
// Licensed under the Apache License, Version 2.0
// SPDX-License-Identifier: Apache-2.0

#include <contract/attributes/attributes.hpp>
#include <contract/detail/detection.hpp>
#include <contract/tag.hpp>

#include <string_view>
#include <type_traits>
#include <utility>

namespace contract {

// How a field's value is physically reached. Exactly one kind per field;
// base import is an orthogonal property (origin, not access).
enum class field_kind {
    member,    // plain data member, reached through a member pointer
    reference, // reference member or custom storage, reached through the tag
    property,  // no physical storage, reached only through user hooks
};

template<class T>
struct member_pointer_traits;

// Member pointer traits normalize "T Owner::*" into owner/storage/value types.
template<class Owner, class Value>
struct member_pointer_traits<Value Owner::*> {
    using owner_type = Owner;
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

// The CONTRACT macro bakes physical access into the per-field tag:
// a tag with a member pointer is a plain member field, a tag with only
// get/set access wraps a reference member, an empty tag is a property.
template<class Tag>
concept tag_has_member_pointer = requires { Tag::contract_member_pointer(); };

template<class Tag, class Owner>
concept tag_has_get = requires(Owner& obj) { Tag::contract_get(obj); };

template<class Tag, class Owner>
constexpr field_kind derive_field_kind() {
    if constexpr (tag_has_member_pointer<Tag>) {
        return field_kind::member;
    } else if constexpr (tag_has_get<Tag, Owner>) {
        return field_kind::reference;
    } else {
        return field_kind::property;
    }
}

// storage/value type resolution per kind.
template<field_kind Kind, class Owner, class Tag, class DeclaredValue>
struct field_types {
    // field_kind::property
    using storage_type = void;
    using value_type = DeclaredValue;
};

template<class Owner, class Tag, class DeclaredValue>
struct field_types<field_kind::member, Owner, Tag, DeclaredValue> {
    using traits = member_pointer_traits<decltype(Tag::contract_member_pointer())>;
    using storage_type = typename traits::storage_type;
    using value_type = typename traits::value_type;
};

template<class Owner, class Tag, class DeclaredValue>
struct field_types<field_kind::reference, Owner, Tag, DeclaredValue> {
    using storage_type = std::remove_cvref_t<decltype(Tag::contract_get(std::declval<Owner&>()))>;
    using value_type = std::remove_cvref_t<storage_type>;
};

} // namespace detail

// The single adapter-facing field descriptor. Identity (name, ids), types,
// access kind, access functions, and attributes all live here; the access
// kind and types are derived from the macro-generated Tag, not declared.
template<
    class Owner,
    class Tag,
    int DeclaredId,
    class Attributes = attributes<>,
    class DeclaredValue = void,
    int EffectiveId = DeclaredId,
    bool Imported = false>
struct field {
    using tag = Tag;
    using owner_type = Owner;
    using attributes_type = Attributes;

    static constexpr field_kind kind = detail::derive_field_kind<Tag, Owner>();
    static constexpr bool is_base_import = Imported;

    static constexpr int declared_id = DeclaredId;
    static constexpr int id = EffectiveId;
    // Total distance from the declared id, accumulated across nested BASE imports.
    static constexpr int base_offset = EffectiveId - DeclaredId;

    static_assert(kind != field_kind::property || !std::is_void_v<DeclaredValue>,
        "PROPERTY fields must carry their declared value type");

    using storage_type = typename detail::field_types<kind, Owner, Tag, DeclaredValue>::storage_type;
    using value_type = typename detail::field_types<kind, Owner, Tag, DeclaredValue>::value_type;

    std::string_view name;
    [[no_unique_address]]
    attributes_type attributes;

    template<class Object>
    static constexpr bool has_custom_get =
        detail::has_free_contract_get<field, Object> ||
        detail::has_free_contract_get_tag<Tag, Object> ||
        detail::has_member_contract_get_tag<Object, Tag>;

    template<class Object, class Value>
    static constexpr bool has_custom_set =
        detail::has_free_contract_set<field, Object, Value> ||
        detail::has_free_contract_set_tag<Tag, Object, Value> ||
        detail::has_member_contract_set_tag<Object, Tag, Value>;

    template<class Object>
    static constexpr bool can_direct_ref = [] {
        if constexpr (kind == field_kind::member) {
            return detail::can_member_ref<Object, Tag::contract_member_pointer()>;
        } else if constexpr (kind == field_kind::reference) {
            return std::is_reference_v<decltype(Tag::contract_get(std::declval<Object&>()))>;
        } else {
            return false;
        }
    }();

    template<class Object>
    constexpr decltype(auto) ref(Object& obj) const
        requires (kind != field_kind::property)
    {
        if constexpr (kind == field_kind::member) {
            constexpr auto member_ptr = Tag::contract_member_pointer();
            return obj.*member_ptr;
        } else {
            return Tag::contract_get(obj);
        }
    }

    template<class Object>
    constexpr decltype(auto) ref(const Object& obj) const
        requires (kind != field_kind::property)
    {
        if constexpr (kind == field_kind::member) {
            constexpr auto member_ptr = Tag::contract_member_pointer();
            return obj.*member_ptr;
        } else {
            return Tag::contract_get(obj);
        }
    }

    template<class Object>
    constexpr decltype(auto) ref(Object&&) const = delete;

    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        if constexpr (detail::has_free_contract_get<field, Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<Tag, Object&>) {
            return contract_get(contract::tag<Tag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<Object, Tag>) {
            return obj.contract_get(contract::tag<Tag>{});
        } else if constexpr (kind != field_kind::property) {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field,obj) ADL hook or member contract_get(tag)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        if constexpr (detail::has_free_contract_get<field, const Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<Tag, const Object&>) {
            return contract_get(contract::tag<Tag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<const Object, Tag>) {
            return obj.contract_get(contract::tag<Tag>{});
        } else if constexpr (kind != field_kind::property) {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field,obj) ADL hook or member contract_get(tag)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object& obj, Value&& value) const {
        if constexpr (detail::has_free_contract_set<field, Object&, Value&&>) {
            contract_set(*this, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_free_contract_set_tag<Tag, Object&, Value&&>) {
            contract_set(contract::tag<Tag>{}, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_member_contract_set_tag<Object, Tag, Value&&>) {
            obj.contract_set(contract::tag<Tag>{}, std::forward<Value>(value));
        } else if constexpr (kind == field_kind::member) {
            ref(obj) = std::forward<Value>(value);
        } else if constexpr (kind == field_kind::reference) {
            Tag::contract_set(obj, std::forward<Value>(value));
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_set(field,obj,value) ADL hook or member contract_set(tag,value)");
        }
    }

    template<class Object, class Value>
    constexpr void set(const Object&, Value&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object&&, Value&&) const = delete;

    // BASE(Type, offset) imports re-instantiate the descriptor with the
    // shifted effective id; access and identity stay with the original owner.
    template<int Offset>
    constexpr auto imported() const {
        using imported_type =
            field<Owner, Tag, DeclaredId, Attributes, DeclaredValue, EffectiveId + Offset, true>;
        return imported_type{name, attributes};
    }
};

template<class Base, int Offset>
struct base {
    using base_type = Base;

    static constexpr int offset = Offset;
};

template<class Owner, class Tag, int Id, class Value = void, class... Attrs>
constexpr auto make_field(std::string_view name, attributes<Attrs...> field_attributes) {
    return field<Owner, Tag, Id, attributes<Attrs...>, Value>{name, std::move(field_attributes)};
}

} // namespace contract
