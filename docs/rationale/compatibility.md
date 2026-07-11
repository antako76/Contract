# CONTRACT Compatibility

This page keeps the versioning and compatibility story in one place.

It is not the public API contract. It explains how to think about source,
schema, and wire compatibility across versions and language modes.

## What Compatibility Means

Compatibility is not one promise. CONTRACT should make the level explicit.

Useful levels:

- source compatibility: the same source model can still be consumed by tools or
  adapters;
- schema compatibility: field ids, names, and shapes can be compared across
  versions;
- wire compatibility: the produced payload remains readable by older or newer
  consumers;
- behavioral compatibility: a bridge preserves the semantic meaning of the
  fields even if the representation changes.

## Versioning Direction

The contract layer should treat versioning as an explicit policy concern.

Useful rules:

- keep field ids stable once published;
- reserve ids that are removed;
- treat field kind changes carefully;
- distinguish optional, repeated, and required-like changes;
- prefer additive changes over destructive changes;
- track base offset collisions and field id reuse as explicit checks.

## C++ Mode Considerations

The implementation and the docs should stay clear about what is guaranteed in
each language mode.

The practical direction is:

- keep the public contract model readable in C++20+;
- keep legacy compatibility only where it is still required by the project;
- avoid turning the documentation into a per-standard code-generation guide;
- express the current support shape in one place instead of scattering it.

If a future compatibility note is needed for a specific standard mode, it should
live here or in a sibling note, not in the public API docs.

## Compatibility Checks

Useful checks include:

- field id uniqueness;
- reserved id ranges;
- base offset collisions;
- type compatibility;
- optional/repeated transitions;
- property field read/write constraints;
- generated schema / bridge checks.

Field id uniqueness is not just a convention - it is a compile-time check.
Two fields sharing an id fail to compile
([`tests/compile_fail/core/duplicate_id_fail.cpp`](../../tests/compile_fail/core/duplicate_id_fail.cpp)):

```cpp
struct DuplicateId {
    int first = 0;
    int second = 0;

    CONTRACT(DuplicateId, (first, 1), (second, 1)) // both use id 1
};
```

```text
error: static assertion failed due to requirement
'contract::detail::unique_field_ids<...>::value':
CONTRACT field ids must be unique after BASE offsets are applied
```

## Design Rule

Keep compatibility explicit, incremental, and checkable.

Do not bury compatibility promises inside individual adapters if they belong to
the contract layer or tooling policy.
