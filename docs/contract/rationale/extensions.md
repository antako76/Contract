# CONTRACT Extension Points

This page collects the extension points that make CONTRACT useful beyond the
core object model.

## Public Query Surface

The core layer exposes a small query API in [`../../../include/contract/definition.hpp`](../../../include/contract/definition.hpp) and [`../../../include/contract/visit.hpp`](../../../include/contract/visit.hpp):

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

The following override shapes stay important:

- physical member override through member hooks;
- physical member override through ADL-discovered free hooks;
- logical/property field override through `contract_get` / `contract_set` and
  [`contract::tag<...>`](../../../include/contract/tag.hpp).

The design rule is the same as before:

- keep access customization local;
- keep it incremental;
- keep it separate from adapter behavior.

## Field Policies

Field-specific adapter behavior belongs in a field policy layer:

```text
field_policy<Adapter, Field>
```

This is the right extension point for:

- PII masking;
- skip rules;
- adapter-specific field behavior;
- field-level debug redaction.

Field policies should not become a second contract model. They are adapter-side
behavior for a specific field.

## Type Codecs

Type-specific adapter behavior belongs in a codec layer:

```text
type_codec<Adapter, T>
```

This is the right extension point for:

- custom wire shapes;
- scalar-like special handling;
- borrowed/owning type behavior;
- container or composite encoding rules;
- adapter-specific type extensions.

Type codecs should stay separate from field policies.

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
- field policy handles field-specific adapter behavior;
- type codec handles type-specific adapter behavior.
