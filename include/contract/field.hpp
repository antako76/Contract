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

namespace detail {

template<class Value, class T>
constexpr decltype(auto) copy_from_volatile_if_needed(T&& value) {
    if constexpr (std::is_volatile_v<std::remove_reference_t<T>>) {
        return Value(std::forward<T>(value));
    } else {
        return std::forward<T>(value);
    }
}

} // namespace detail

template<class T>
struct member_pointer_traits;

// Member pointer traits normalize "T Owner::*" into owner/storage/value types.
template<class Owner, class Value>
struct member_pointer_traits<Value Owner::*> {
    using owner_type = Owner;
    using storage_type = Value;
    using value_type = std::remove_cvref_t<Value>;
};

template<auto MemberPtr, class Tag, int Id, class Attributes = attributes<>>
struct member_field {
    using tag = Tag;
    using member_pointer_type = decltype(MemberPtr);
    using member_traits = member_pointer_traits<member_pointer_type>;
    using owner_type = typename member_traits::owner_type;
    using storage_type = typename member_traits::storage_type;
    using value_type = typename member_traits::value_type;
    using attributes_type = Attributes;
    static constexpr bool is_member_field = true;
    static constexpr bool is_property_field = false;
    static constexpr bool is_reference_field = false;
    static constexpr bool is_base_import = false;

    static constexpr int id = Id;
    static constexpr int base_offset = 0;
    static constexpr auto member_pointer = MemberPtr;

    std::string_view name;
    [[no_unique_address]]
    attributes_type attributes;

    template<class Object>
    static constexpr bool has_custom_get =
        detail::has_free_contract_get<member_field, Object> ||
        detail::has_free_contract_get_tag<Tag, Object> ||
        detail::has_member_contract_get_tag<Object, Tag>;

    template<class Object, class Value>
    static constexpr bool has_custom_set =
        detail::has_free_contract_set<member_field, Object, Value> ||
        detail::has_free_contract_set_tag<Tag, Object, Value> ||
        detail::has_member_contract_set_tag<Object, Tag, Value>;

    template<class Object>
    static constexpr bool can_direct_ref = detail::can_member_ref<Object, MemberPtr>;

    template<class Object>
    constexpr decltype(auto) ref(Object& obj) const {
        return obj.*MemberPtr;
    }

    template<class Object>
    constexpr decltype(auto) ref(const Object& obj) const {
        return obj.*MemberPtr;
    }

    template<class Object>
    constexpr decltype(auto) ref(Object&&) const = delete;

    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        if constexpr (detail::has_free_contract_get<member_field, Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<Tag, Object&>) {
            return contract_get(contract::tag<Tag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<Object, Tag>) {
            return obj.contract_get(contract::tag<Tag>{});
        } else {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        }
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        if constexpr (detail::has_free_contract_get<member_field, const Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<Tag, const Object&>) {
            return contract_get(contract::tag<Tag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<const Object, Tag>) {
            return obj.contract_get(contract::tag<Tag>{});
        } else {
            decltype(auto) value = ref(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        }
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object& obj, Value&& value) const {
        if constexpr (detail::has_free_contract_set<member_field, Object&, Value&&>) {
            contract_set(*this, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_free_contract_set_tag<Tag, Object&, Value&&>) {
            contract_set(contract::tag<Tag>{}, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_member_contract_set_tag<Object, Tag, Value&&>) {
            obj.contract_set(contract::tag<Tag>{}, std::forward<Value>(value));
        } else {
            ref(obj) = std::forward<Value>(value);
        }
    }

    template<class Object, class Value>
    constexpr void set(const Object&, Value&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object&&, Value&&) const = delete;
};

template<class Owner, class FieldTag, int Id, class Access, class Attributes = attributes<>>
struct reference_field {
    using tag = FieldTag;
    using owner_type = Owner;
    using access_result_type = decltype(std::declval<Access&>().get(std::declval<Owner&>()));
    using storage_type = std::remove_cvref_t<access_result_type>;
    using value_type = std::remove_cvref_t<storage_type>;
    using attributes_type = Attributes;
    static constexpr bool is_member_field = false;
    static constexpr bool is_property_field = false;
    static constexpr bool is_reference_field = true;
    static constexpr bool is_base_import = false;

    static constexpr int id = Id;
    static constexpr int base_offset = 0;

    Access access;
    std::string_view name;
    [[no_unique_address]]
    attributes_type attributes;

    template<class Object>
    static constexpr bool has_custom_get =
        detail::has_free_contract_get<reference_field, Object> ||
        detail::has_free_contract_get_tag<FieldTag, Object> ||
        detail::has_member_contract_get_tag<Object, FieldTag>;

    template<class Object, class Value>
    static constexpr bool has_custom_set =
        detail::has_free_contract_set<reference_field, Object, Value> ||
        detail::has_free_contract_set_tag<FieldTag, Object, Value> ||
        detail::has_member_contract_set_tag<Object, FieldTag, Value>;

    template<class Object>
    static constexpr bool can_direct_ref =
        std::is_reference_v<decltype(std::declval<Access&>().get(std::declval<Object&>()))>;

    template<class Object>
    constexpr decltype(auto) ref(Object& obj) const {
        return access.get(obj);
    }

    template<class Object>
    constexpr decltype(auto) ref(const Object& obj) const {
        return access.get(obj);
    }

    template<class Object>
    constexpr decltype(auto) ref(Object&&) const = delete;

    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        if constexpr (detail::has_free_contract_get<reference_field, Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<FieldTag, Object&>) {
            return contract_get(contract::tag<FieldTag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<Object, FieldTag>) {
            return obj.contract_get(contract::tag<FieldTag>{});
        } else {
            decltype(auto) value = access.get(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        }
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        if constexpr (detail::has_free_contract_get<reference_field, const Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<FieldTag, const Object&>) {
            return contract_get(contract::tag<FieldTag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<const Object, FieldTag>) {
            return obj.contract_get(contract::tag<FieldTag>{});
        } else {
            decltype(auto) value = access.get(obj);
            return detail::copy_from_volatile_if_needed<value_type>(value);
        }
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object& obj, Value&& value) const {
        if constexpr (detail::has_free_contract_set<reference_field, Object&, Value&&>) {
            contract_set(*this, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_free_contract_set_tag<FieldTag, Object&, Value&&>) {
            contract_set(contract::tag<FieldTag>{}, obj, std::forward<Value>(value));
        } else if constexpr (detail::has_member_contract_set_tag<Object, FieldTag, Value&&>) {
            obj.contract_set(contract::tag<FieldTag>{}, std::forward<Value>(value));
        } else {
            access.set(obj, std::forward<Value>(value));
        }
    }

    template<class Object, class Value>
    constexpr void set(const Object&, Value&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object&&, Value&&) const = delete;
};

template<class Owner, class FieldTag, int Id, class Value, class Attributes = attributes<>>
struct property_field {
    using tag = FieldTag;
    using owner_type = Owner;
    using storage_type = void;
    using value_type = Value;
    using attributes_type = Attributes;
    static constexpr bool is_member_field = false;
    static constexpr bool is_property_field = true;
    static constexpr bool is_reference_field = false;
    static constexpr bool is_base_import = false;

    static constexpr int id = Id;
    static constexpr int base_offset = 0;

    std::string_view name;
    [[no_unique_address]]
    attributes_type attributes;

    // Property fields always rely on hooks; they have no direct storage ref.
    template<class Object>
    static constexpr bool has_custom_get =
        detail::has_free_contract_get<property_field, Object> ||
        detail::has_free_contract_get_tag<FieldTag, Object> ||
        detail::has_member_contract_get_tag<Object, FieldTag>;

    template<class Object, class SetValue>
    static constexpr bool has_custom_set =
        detail::has_free_contract_set<property_field, Object, SetValue> ||
        detail::has_free_contract_set_tag<FieldTag, Object, SetValue> ||
        detail::has_member_contract_set_tag<Object, FieldTag, SetValue>;

    // Property fields intentionally never expose direct member refs.
    template<class Object>
    static constexpr bool can_direct_ref = false;

    // Read/write is resolved entirely through hooks or contract tags.
    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        if constexpr (detail::has_free_contract_get<property_field, Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<FieldTag, Object&>) {
            return contract_get(contract::tag<FieldTag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<Object, FieldTag>) {
            return obj.contract_get(contract::tag<FieldTag>{});
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field,obj) ADL hook or member contract_get(tag)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        if constexpr (detail::has_free_contract_get<property_field, const Object&>) {
            return contract_get(*this, obj);
        } else if constexpr (detail::has_free_contract_get_tag<FieldTag, const Object&>) {
            return contract_get(contract::tag<FieldTag>{}, obj);
        } else if constexpr (detail::has_member_contract_get_tag<const Object, FieldTag>) {
            return obj.contract_get(contract::tag<FieldTag>{});
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_get(field,obj) ADL hook or member contract_get(tag)");
        }
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class SetValue>
    constexpr void set(Object& obj, SetValue&& value) const {
        if constexpr (detail::has_free_contract_set<property_field, Object&, SetValue&&>) {
            contract_set(*this, obj, std::forward<SetValue>(value));
        } else if constexpr (detail::has_free_contract_set_tag<FieldTag, Object&, SetValue&&>) {
            contract_set(contract::tag<FieldTag>{}, obj, std::forward<SetValue>(value));
        } else if constexpr (detail::has_member_contract_set_tag<Object, FieldTag, SetValue&&>) {
            obj.contract_set(contract::tag<FieldTag>{}, std::forward<SetValue>(value));
        } else {
            static_assert(sizeof(Object) == 0,
                "PROPERTY field requires contract_set(field,obj,value) ADL hook or member contract_set(tag,value)");
        }
    }

    template<class Object, class SetValue>
    constexpr void set(const Object&, SetValue&&) const = delete;

    template<class Object, class SetValue>
    constexpr void set(Object&&, SetValue&&) const = delete;
};

template<class Base, int Offset>
struct base {
    using base_type = Base;

    static constexpr int offset = Offset;
};

// Adapter-facing descriptor for a field imported through BASE(Type, offset).
// Storage access stays delegated to the original field; only the effective
// contract id changes.
template<class Field, int Offset>
struct offset_field {
    using tag = typename Field::tag;
    using owner_type = typename Field::owner_type;
    using storage_type = typename Field::storage_type;
    using value_type = typename Field::value_type;
    using attributes_type = typename Field::attributes_type;
    static constexpr bool is_member_field = Field::is_member_field;
    static constexpr bool is_property_field = Field::is_property_field;
    static constexpr bool is_reference_field = Field::is_reference_field;
    static constexpr bool is_base_import = true;

    static constexpr int id = Field::id + Offset;
    static constexpr int base_offset = Offset;

    Field field;
    std::string_view name;
    [[no_unique_address]]
    attributes_type attributes;

    // The imported field is stored by value; only the effective id changes.
    constexpr explicit offset_field(Field field)
        : field(field)
        , name(field.name)
        , attributes(field.attributes) {}

    template<class Object>
    static constexpr bool has_custom_get = Field::template has_custom_get<Object>;

    template<class Object, class Value>
    static constexpr bool has_custom_set = Field::template has_custom_set<Object, Value>;

    template<class Object>
    static constexpr bool can_direct_ref = Field::template can_direct_ref<Object>;

    template<class Object>
    constexpr decltype(auto) ref(Object& obj) const {
        return field.ref(obj);
    }

    template<class Object>
    constexpr decltype(auto) ref(const Object& obj) const {
        return field.ref(obj);
    }

    template<class Object>
    constexpr decltype(auto) ref(Object&&) const = delete;

    template<class Object>
    constexpr decltype(auto) get(Object& obj) const {
        return field.get(obj);
    }

    template<class Object>
    constexpr decltype(auto) get(const Object& obj) const {
        return field.get(obj);
    }

    template<class Object>
    constexpr decltype(auto) get(Object&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object& obj, Value&& value) const {
        field.set(obj, std::forward<Value>(value));
    }

    template<class Object, class Value>
    constexpr void set(const Object&, Value&&) const = delete;

    template<class Object, class Value>
    constexpr void set(Object&&, Value&&) const = delete;
};

template<class Owner, class FieldTag, int Id, class Access, class... Attrs>
constexpr auto make_access_field_with_attributes(
    std::string_view name,
    Access access,
    attributes<Attrs...> field_attributes) {
    if constexpr (requires { Access::member_pointer(); }) {
        constexpr auto member_ptr = Access::member_pointer();
        using field_type = member_field<member_ptr, FieldTag, Id, attributes<Attrs...>>;
        return field_type{name, std::move(field_attributes)};
    } else {
        // Reference members cannot form a member pointer, so they become reference_field.
        using field_type = reference_field<Owner, FieldTag, Id, Access, attributes<Attrs...>>;
        return field_type{access, name, std::move(field_attributes)};
    }
}

template<class Owner, class FieldTag, int Id, class Value, class... Attrs>
constexpr auto make_property_field(std::string_view name, Attrs&&... attrs) {
    // Property descriptors store only metadata; value access is hook-driven.
    auto field_attributes = make_field_attributes(
        describe_attribute(std::forward<Attrs>(attrs), {})...);
    using field_type = property_field<Owner, FieldTag, Id, Value, decltype(field_attributes)>;
    return field_type{name, std::move(field_attributes)};
}

template<class Owner, class FieldTag, int Id, class Value, class... Attrs>
constexpr auto make_property_field_with_attributes(
    std::string_view name,
    attributes<Attrs...> field_attributes) {
    using field_type = property_field<Owner, FieldTag, Id, Value, attributes<Attrs...>>;
    return field_type{name, std::move(field_attributes)};
}

} // namespace contract
