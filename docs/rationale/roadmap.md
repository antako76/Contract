# CONTRACT Roadmap

This page collects the practical implementation direction for the current
CONTRACT effort.

It is not the canonical API surface. It describes what the library should prove
first and which adapter capabilities matter early.

## Current Feature Plan

The near-term plan is:

1. Finish the console/debug adapter so schema, debug, and value views stay
   aligned with the current contract model.
2. Keep the binary adapter stable and boring.
3. Add a protobuf-like adapter that can export C++ contracts to protobuf wire
   shape and schema output.
4. Keep schema and compatibility checks explicit.
5. Keep benchmarks on handwritten baselines so performance claims stay honest.

The rule is to finish one adapter path cleanly before adding a new one that
depends on the same contract rules.

## MVP Shape

The initial goal is to prove contract-layer value, not to claim the fastest
serializer in the world.

The minimal useful shape is:

1. `CONTRACT(...)` for physical fields.
2. `BASE(Type, offset)`.
3. `PROPERTY(name, id, type)`.
4. C++20 descriptor core.
5. Debug dump adapter.
6. Binary writer/reader adapter.
7. Schema dump and id collision checks.
8. Protobuf-like writer or `.proto` generator/checker prototype.
9. Access customization, `type_codec`, and `field_policy`.
10. Benchmarks against handwritten adapters.

## Type Support Targets

The baseline adapter support should cover:

```text
bool;
integers;
floating point;
enums;
std::string;
std::string_view for writing;
std::vector<T>;
std::array<T, N>;
std::optional<T>;
nested CONTRACT objects;
BASE-imported fields;
PROPERTY fields.
```

Later targets may include:

```text
std::variant;
std::map / std::unordered_map;
std::chrono;
std::span for writing and parsing helpers;
smart pointers with explicit policy;
reference tracking as adapter feature;
polymorphism only as explicit adapter extension.
```

## Performance Direction

The contract layer should compile close to handwritten code.

That requires:

- descriptors known at compile time;
- member pointers where possible;
- inline traversal;
- no type erasure;
- no runtime registry;
- no allocations in core;
- adapter calls visible to the optimizer.

The benchmark framing should compare CONTRACT adapters to handwritten code that
performs the same semantic work.

## Schema and Compatibility Direction

Schema and compatibility checks should be first-class adapter or tooling
behaviors, not core runtime policy.

Useful checks:

- field id uniqueness;
- base offset collisions;
- field id reuse;
- type compatibility;
- reserved id ranges;
- optional/repeated changes;
- property field read/write constraints.

## Adapter Pack Direction

The early adapter pack should prove the value of the model:

- binary writer/reader;
- debug dump / console audit view;
- schema dump;
- schema checker;
- later protobuf-like or export bridges.

The point is not to replace every serializer. The point is to show that one
contract can feed many adapters.
