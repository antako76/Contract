# CONTRACT Extension Points

This page collects the extension points that make CONTRACT useful beyond the
core object model.

## Public Query Surface

The core layer exposes a small query API in [`../../include/contract/definition.hpp`](../../include/contract/definition.hpp) and [`../../include/contract/visit.hpp`](../../include/contract/visit.hpp):

```cpp
contract::contract_of<T>();
contract::flattened_fields_of<T>();
contract::type_name<T>();
contract::for_each_field<T>(fn);
contract::field_count<T>();
contract::field_at<Index, T>();
contract::visit(object, adapter);
```

These helpers make the declared contract discoverable without requiring
adapters to depend on internal tuple storage.

## Access Customization

Default access is direct member access when possible.

The current core model classifies each field into one of three kinds:

- `member` - the descriptor carries a member pointer and can use direct storage access;
- `reference` - `REFERENCE(...)` generates a descriptor with direct reference access;
- `property` - the descriptor is hook-driven only.

The following override shapes stay important:

- physical member override through member hooks;
- physical member override through ADL-discovered free hooks;
- logical/property field override through `contract_get` / `contract_set` and
  the concrete field descriptor type.

The design rule is the same as before:

- keep access customization local;
- keep it incremental;
- keep it separate from adapter behavior.

A member hook overrides get/set for one field without changing its physical
storage type
([`tests/contract_test_types.hpp`](../../tests/contract_test_types.hpp)):

```cpp
struct HookedMetric {
    int raw_count = 0;

    CONTRACT(HookedMetric, (raw_count, 1))

    int contract_get(const contract_fields::raw_count&) const {
        return raw_count * 10;
    }

    template<class Value>
    void contract_set(const contract_fields::raw_count&, Value&& value) {
        raw_count = static_cast<int>(std::forward<Value>(value)) / 10;
    }
};
```

This is still a `member` field from the core's point of view: the descriptor
carries the member pointer and can fall back to direct storage
when no hook overrides it.

An ADL-discovered free function does the same thing without adding members to
the type - useful when the type itself cannot be changed:

```cpp
template<class Field>
int contract_get(const Field&, const FreeHookMetric& metric) {
    return metric.raw_count * 100;
}
```

For property-style fields, the same pattern applies but the tag is empty and
the descriptor must be given an explicit logical value type in the contract
declaration. In that case the core never falls back to raw storage access; it
expects hooks.

## Field Policies

Field-specific adapter behavior is expressed through the attribute system:
attributes declared on a field in `CONTRACT(...)`, resolved per adapter
through visibility and `attribute_rules`. See
[`attributes/README.md`](../attributes/README.md) for the full model.

This is the extension point actually used today for:

- PII masking and redaction - `contract::security::secret()` /
  `sensitive()`, resolved by the console and JSON adapters
  ([`attributes/security.md`](../attributes/security.md));
- skip rules - `contract::security::no_log()`;
- adapter-specific field behavior - an adapter declares its own
  `attribute_rules` over the vocabularies it cares about
  ([`attributes/adapters.md`](../attributes/adapters.md)).

Attributes should not become a second contract model. They are adapter-side
behavior for a specific field, resolved through the shared attribute
pipeline, not a parallel per-adapter dispatch mechanism.

## Type Codecs

Type-specific adapter behavior belongs in a `codec<T>` specialization inside
the owning adapter namespace - for example
`contract::adapters::binary::codec<std::string>` or
`contract::adapters::protobuf::codec<std::vector<T>>`. Each adapter family
specializes `codec<T>` for the value types it supports; see
[`adapters/README.md`](../adapters/README.md#field-codec-and-traversal-rules)
for the field-aware vs. value-only overload split every family follows.

This is the extension point actually used today for:

- custom wire shapes (e.g. protobuf's length-delimited `bytes`/`string`
  encoding vs. binary's length-prefixed form);
- scalar-like special handling (e.g. `char[N]` mapping to a fixed-size
  `bytes` field instead of a repeated block);
- borrowed/owning type behavior;
- container or composite encoding rules (`std::vector`, `std::variant`,
  `std::map`, and similar).

Codecs should stay separate from field-level attribute handling: a codec
knows the shape of a type; attribute resolution decides whether and how a
field's value reaches that codec at all.

In general, adapters should expose shared low-level primitives, and codecs
should compose those primitives into a type-specific shape. Formatting helpers
should stay pure and stateless. That keeps new container or composite types
cheap to add without forcing the adapter to predict every possible schema in
advance.

## Schema and Compatibility Checks

Some adapters and tools need to validate or inspect the contract rather than
just traverse it.

Useful checks and tools include:

- base offset collision checks;
- field id uniqueness checks;
- schema dump tools;
- schema compatibility checks;
- generated format checks such as protobuf-style adapters.

These are adapter or tooling responsibilities, not core runtime policy.

## Design Rule

Keep the core compact, and make extension points explicit:

- core exposes the graph and query surface;
- adapters own runtime behavior;
- attribute resolution handles field-specific adapter behavior;
- `codec<T>` handles type-specific adapter behavior.
