# Field Identity Refactor Specification

**Status:** implemented. This document defines the resulting core model. It
deliberately does not preserve the former field-tag API.

## Decision

`contract::field` is the single descriptor and the single customization key for
a declared field. The implementation removes the separate per-field `Tag`
type from `field` and from field access dispatch.

`contract::tag<Owner>` remains the mechanism for discovering a contract
definition through its hidden friend. This specification changes only tags used
to identify and access individual fields.

## Problem

Today `CONTRACT` creates both a per-field tag and a `field<Owner, Tag, ...>`
descriptor. For a physical member, the macro writes generated `get`, `set`,
and member-pointer functions into the tag. `field` then probes those functions
to rediscover the field kind, types, and fallback access path.

The macro already knows the physical member and its id. Encoding that
information as functions and deriving it again duplicates the declaration
model. A property tag is empty, but exists only as a hook-selection key.

## Target Model

There is one declared descriptor type per declared field:

```cpp
// Illustrative spelling; internal template-parameter layout is not public.
using service = contract::field<PaymentConfig, 1, &PaymentConfig::service, /* attrs */>;
using duration_ns = contract::field<PaymentConfig, 10, contract::no_member,
    /* attrs */, std::uint64_t>;
```

The `CONTRACT` macro exposes those types as aliases in a nested
`contract_fields` type. For member and property declarations it is a name
scope containing aliases only. A `REFERENCE(...)` entry is the one exception:
it declares its descriptor type there because that descriptor must carry the
otherwise unrepresentable `object.member` fallback operation.

The declaration DSL has three access forms:

```cpp
(name, id, attrs...)                 // non-reference physical member
REFERENCE(name, id, attrs...)        // T& data member
PROPERTY(name, id, Value, attrs...)  // hook-only logical property
```

`REFERENCE` is explicit because C++ does not permit `&Owner::member` for a
reference data member. Its type is obtained internally from
`decltype(Owner::member)`, which is valid in the nested descriptor declaration;
it is not inferred by probing generated code.

```cpp
struct PaymentConfig {
    std::string service;

    CONTRACT(PaymentConfig,
        (service, 1),
        PROPERTY(duration_ns, 10, std::uint64_t))

    std::uint64_t contract_get(const contract_fields::duration_ns&) const;
    template<class Value>
    void contract_set(const contract_fields::duration_ns&, Value&&);
};
```

The exact alias spelling is public. The internal template arguments used to
represent direct member or reference access are not.

## Access Kinds

`field_kind` remains a public descriptor property. It is determined from the
declared access form, not by probing a generated tag.

| Declaration form | `Field::kind` | Direct access information |
| --- | --- | --- |
| non-reference physical member | `member` | `&Owner::member` |
| `REFERENCE(name, id, ...)` | `reference` | macro-generated direct reference thunk |
| `PROPERTY(name, id, Value, ...)` | `property` | none |

For a non-reference physical member, the member pointer is the sole source of
truth for `storage_type`, `value_type`, `ref`, and the fallback write path.

For a reference member, C++ has no pointer-to-reference-member: `&Owner::ref`
is ill-formed. The macro therefore supplies a direct get/set thunk to the
descriptor implementation. That thunk is an internal implementation detail,
not a public tag, trait, or new customization point. It exists only to express
the source-level operation `object.ref` that a member pointer cannot express.

For a property, `storage_type` is `void`, `value_type` is the `PROPERTY`
declared type, and no direct fallback exists.

## Field-Centred Hooks

`field::get` and `field::set` retain their current role as the one access
dispatcher. Their supported customization forms become:

```cpp
// ADL hooks
contract_get(field, object);
contract_set(field, object, value);

// Owner member hooks
object.contract_get(field);
object.contract_set(field, value);
```

Here `field` has the concrete declared descriptor type, for example
`const PaymentConfig::contract_fields::duration_ns&`.

Dispatch order is fixed:

1. free `contract_get(field, object)` / `contract_set(field, object, value)`;
2. owner member `object.contract_get(field)` /
   `object.contract_set(field, value)`;
3. direct descriptor fallback, when the access kind has one.

The direct fallback is member-pointer access for `member` and the internal
reference thunk for `reference`. A `property` without an applicable hook is a
compile-time error.

Hooks may override the direct fallback for physical and reference fields, just
as they do today. `has_custom_get`, `has_custom_set`, and `can_direct_ref`
remain available with their existing meanings, but are computed without tag
probes.

The following forms are removed:

```cpp
contract_get(contract::tag<FieldTag>{}, object);
contract_set(contract::tag<FieldTag>{}, object, value);
object.contract_get(contract::tag<FieldTag>{});
object.contract_set(contract::tag<FieldTag>{}, value);
Tag::contract_get(object);
Tag::contract_set(object, value);
Tag::contract_member_pointer();
Field::tag;
```

## Base Imports

`BASE(Type, offset)` creates an imported descriptor projection with a shifted
effective id. It must not change the identity used for access hooks.

An imported descriptor therefore retains a `declared_field` referring to the
original declared descriptor. Its `get`, `set`, `ref`, custom-hook lookup, and
attributes delegate to that declared descriptor; only `id`, `base_offset`, and
`is_base_import` differ. This preserves property and custom-access behavior
across arbitrary nested base imports without reviving a separate tag type.

## Public Boundary

Public core surface after this change:

- `field` exposes owner, declared/effective ids, name, attributes, kind,
  storage/value types, access operations, and import projection.
- `Owner::contract_fields::name` names the declared descriptor type for hooks
  and compile-time use.
- `field_kind`, `contract_of`, `field_at`, and flattened traversal keep their
  roles.

The following are internal only:

- the representation of direct physical access inside `field`;
- the generated reference-member thunk;
- imported-descriptor implementation types.

No compatibility layer is provided for `Owner::contract_field::name`, tag
hooks, `Field::tag`, or tag-derived field classification. This is an intentional
source-breaking core simplification.

## Invariants

- A declared field has exactly one descriptor type and one hook identity.
- The macro supplies the member pointer directly whenever C++ permits one.
- `field` never infers an access kind by testing for generated functions.
- Properties have an explicit logical value type.
- Only `field::get` and `field::set` perform hook dispatch.
- Imported fields keep the declared descriptor identity for access and hooks.
- The descriptor and its aliases carry no additional runtime state beyond the
  existing field name and attributes.

## Acceptance Criteria

Implementation is accepted when all of the following hold:

1. Generated ordinary member fields contain no per-field generated struct or
   access functions; their descriptor receives a direct member pointer.
2. Property hooks compile and run when keyed by
   `Owner::contract_fields::property_name` directly, without `contract::tag`.
3. `REFERENCE(...)` fallback access and direct custom overrides both work,
   including `ref` eligibility.
4. Member hooks and free ADL hooks override direct fallback with the documented
   order.
5. Nested `BASE` imports preserve hook behavior while shifting only effective
   ids and import metadata.
6. `field.hpp` contains no `Tag` template parameter, tag-function probes, or
   tag-based access fallback.
7. Core unit tests, adapter tests, compile-fail tests, and examples are updated
   to the new hook syntax and pass under the repository C++20 build.

## Migration Scope

The implementation may update all in-repository contracts and tests in the
same change. External source compatibility is explicitly out of scope. Adapter
APIs remain descriptor-based; they should require no semantic change beyond
the replacement of removed `Field::tag` usage, if any.
