# CONTRACT Core

This page describes the core contract metadata layer: the structural model of
[`CONTRACT`](../include/contract/contract.hpp) definitions before any adapter turns them into bytes or text.

## Responsibilities

- Define fields, ids, names, and access capabilities.
- Provide compile-time traversal and validation.
- Stay free of adapter formatting policy.

## Contract Object Model

The core layer models one `CONTRACT` definition per type.

- one definition describes one owner type;
- it owns the field list for that type;
- fields may be physical members, properties, bases, or custom accessors;
- the definition graph is what adapters traverse and render or encode.

This is the class-level contract model: the structural description of how a type
maps to named, typed fields. Adapters consume this model; they do not define it.

## What `CONTRACT(...)` Adds

The macro creates declared field descriptors and returns a
`contract::definition<Owner, ...>` that contains them. It also exposes their
types through the nested `contract_fields` scope.

For a member field like `(service, 1)`, the descriptor receives the physical
member pointer directly:

```cpp
using service = decltype(contract::make_member_field<
    PaymentConfig, 1, &PaymentConfig::service>("service", {}));
```

Properties have no physical member and use their declared logical type:

```cpp
using duration_ns = decltype(contract::make_property_field<
    PaymentConfig, 10, std::uint64_t>("duration_ns", {}));
```

Reference members are explicit: `REFERENCE(value, id)` generates the reference
descriptor itself. It derives its type from `decltype(Owner::value)` and keeps
the direct `object.value` fallback internally.

## Core Concepts

The core layer revolves around a small set of types:

- `definition<Owner, Attributes, Fields...>` holds the contract graph for one
  owner type (the `Attributes` slot carries the contract-level `ATTRS(...)` pack);
- `owner_type` identifies the described class;
- `fields` is the ordered field tuple for that class;
- `type_name` is the public contract name used by adapters and debug output.

The current contract object is materialized with [`make_contract(type_name,
fields...)`](../include/contract/definition.hpp), and discovered through [`contract_of<T>()`](../include/contract/definition.hpp).

Field entries can describe different access shapes:

- physical member fields;
- property fields backed by `contract_get` / `contract_set`;
- base imports through `BASE(Type, offset)`;
- imported fields produced by base flattening.

The core layer also provides flattening helpers so adapters can consume one
uniform traversal view instead of special-casing base imports in every adapter:

- `contract_of<T>()` resolves the contract definition for `T`;
- `flattened_fields_of<T>()` returns the adapter-facing flattened field tuple;
- `type_name<T>()` returns the public contract name for `T`.

Base imports are represented with `base<Type, offset>` entries and flattened
into imported field descriptors before adapters see them. The imported field
keeps the original tag, name, attributes, and access kind, but its effective id
is shifted by the accumulated base offset. That keeps offset handling in the
core layer rather than duplicating it in every adapter.

The important rule is that core describes the graph, not the runtime policy.
Traversal order, formatting, encoding, and stateful render behavior belong to
adapters.

## Related Docs

- [`adapters/README.md`](adapters/README.md)
- [`rationale/extensions.md`](rationale/extensions.md)
- [`reference/examples.md`](reference/examples.md)
- [`reference/benchmarks.md`](reference/benchmarks.md)

## Public Core Surface

The core layer exposes a small query surface that adapters build on:

- [`contract_of<T>()`](../include/contract/definition.hpp) resolves the contract definition for `T`;
- [`flattened_fields_of<T>()`](../include/contract/definition.hpp) returns the adapter-facing flattened field tuple;
- [`type_name<T>()`](../include/contract/definition.hpp) returns the public contract name for `T` as a `string_view`.
- [`for_each_field<T>(fn)`](../include/contract/visit.hpp) iterates the flattened field descriptors in declaration order;
- [`field_count<T>()`](../include/contract/visit.hpp) returns the number of flattened fields;
- [`field_at<Index, T>()`](../include/contract/visit.hpp) returns one flattened field descriptor by index;
- [`visit(object, adapter)`](../include/contract/visit.hpp) dispatches each flattened field into an adapter;
- [`dispatch_field_by_id<T>(id, fn)`](../include/contract/visit.hpp) finds the declared field whose effective id matches a runtime value and calls `fn(field)`, or returns `false` without calling `fn`;
- [`dispatch_field_by_name<T>(key, fn)`](../include/contract/visit.hpp) does the same keyed by field name, calling `fn(field, index)`.

The last two exist because self-describing wire formats (protobuf, compact,
yaml) all need to map a runtime id/key from the wire to the matching
compile-time field before they can decode it. Each adapter used to implement
this scan itself; all three shared the same bug (restarting the scan from
field 0 on every incoming field, quadratic in field count). They now share
one fused-fold implementation instead.

These helpers are type-first. They make the declared contract discoverable
without requiring adapters to depend on internal tuple storage or legacy
visitor-style macros.

## Generic Traversal

`for_each_field<T>(fn)` is the simplest way to apply one operation to every
field in a contract object without writing a dedicated adapter.

It is useful when you want to:

- collect field names for diagnostics or debug output;
- build a schema fingerprint;
- compare or validate all fields with one generic loop;
- consume field values in a benchmark so the compiler cannot discard the work.

The callback receives the flattened field descriptors for `T`. That means the
caller can inspect metadata such as `field.name`, `field.id`, or attributes, and
can also read the actual value with `field.get(object)`.

Example: gather the public field names in declaration order.

```cpp
template<class T>
std::vector<std::string_view> field_names() {
    std::vector<std::string_view> names;

    contract::for_each_field<T>([&](const auto&... fields) {
        (names.push_back(fields.name), ...);
    });

    return names;
}
```

Example: visit every field value with one callable.

```cpp
template<class T, class Fn>
void visit_values(const T& object, Fn&& fn) {
    contract::for_each_field<T>([&](const auto&... fields) {
        (fn(fields, fields.get(object)), ...);
    });
}
```

The benchmark code uses this shape to keep a generic sink alive after each
measurement: one field traversal, one value touch per field, and no manual
specialization per type.

## Access Semantics

Field descriptors expose both the exact storage type and the adapter-facing
value type:

```text
Field::owner_type    class that owns the field
Field::kind          member / reference / property
Field::declared_id   id written in the contract declaration
Field::id            effective id after BASE flattening
Field::base_offset   imported id shift accumulated across BASE layers
Field::storage_type  exact declared member type, preserving const/volatile
Field::value_type    normalized logical type for adapters
Field::is_base_import
Field::has_custom_get<Object>
Field::has_custom_set<Object, Value>
Field::can_direct_ref<Object>   detection-based direct-ref capability
```

For example, a `volatile unsigned long long` counter has
`storage_type = volatile unsigned long long` and
`value_type = unsigned long long`.

`Field::get(obj)` returns the adapter-facing value. `Field::ref(obj)` returns
raw storage access and preserves qualifiers such as `volatile`.
`Field::set(obj, value)` writes physical member fields directly.

Physical fields can override adapter-facing access with member hooks that take
their concrete descriptor type:

```cpp
contract_get(const contract_fields::name&)
contract_set(const contract_fields::name&, value)
```

If the class cannot be changed, define ADL-discovered free hooks in the same
namespace as the type:

```cpp
contract_get(field, object)
contract_set(field, object, value)
```

Resolution order:

```text
member field
1. free descriptor hook
2. member descriptor hook
3. direct fallback on the physical member
```

```text
reference field
1. free descriptor hook
2. member descriptor hook
3. fallback to generated direct reference access
```

```text
property field
1. free descriptor hook
2. member descriptor hook
3. no direct fallback
```

Property fields use `PROPERTY(name, id, type)` and must provide either free or
member descriptor hooks:

```cpp
contract_get(field, object)
contract_set(field, object, value)
```

Property fields have `storage_type = void` and `value_type = type`.

Examples:

```cpp
// member field
struct User {
    std::uint64_t id;

    std::uint64_t contract_get(const contract_fields::id&) const;
    template<class Value>
    void contract_set(const contract_fields::id&, Value&&);
};
```

```cpp
// reference field
struct Wrapper {
    Target& value;
};

CONTRACT(Wrapper, REFERENCE(value, 1))

Target& contract_get(const Wrapper::contract_fields::value&, Wrapper& object);
void contract_set(const Wrapper::contract_fields::value&, Wrapper& object, Target& value);
```

```cpp
// property field
struct Metric {
    PROPERTY(raw_count, 1, int)

    int contract_get(const contract_fields::raw_count&) const;
    template<class Value>
    void contract_set(const contract_fields::raw_count&, Value&&);
};
```

Core validation is compile-time only. Invalid contracts fail compilation; the
core does not provide runtime validation, runtime registries, or dynamic
checks. Exception policy for the runtime layers is documented in
[`rationale/errors.md`](rationale/errors.md).

P0 core checks:

- `BASE(Type, offset)` must reference a real, accessible, unambiguous base.
- Flattened field ids must be positive.
- Flattened field ids must be unique after base offsets are applied.
