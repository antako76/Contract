# CONTRACT Positioning

This page explains why CONTRACT exists and what problem it solves.

## Core Idea

`CONTRACT` gives serializers and adapters a compile-time data contract.

Native C++ structs own the data. A `CONTRACT(...)` block declares stable field
metadata once: field ids, names, generated field descriptors, field kinds,
base imports, physical fields, reference fields, and logical properties.
Adapters then use that typed compile-time contract to write protobuf, binary,
JSON/debug, backup, schema, export, or validation logic.

```text
Data classes own data.
CONTRACT exposes compile-time contract.
Adapters own behavior and runtime cost.
```

## What CONTRACT Replaces

Without a shared contract layer, each data class tends to accumulate repeated
format-specific methods:

```text
toProto / fromProto / toJson / debugPrint / writeBackup / logFields / schema checks
```

That grows as:

```text
N data classes x M formats = N*M handwritten mappings
```

With `CONTRACT`, the shape becomes:

```text
N contracts + M adapters
```

The point is not to hide the data. The point is to declare the stable schema
once and let adapters reuse it.

## Why the Name Matters

`VISITOR` describes an implementation pattern.
`CONTRACT` describes the engineering boundary:

- stable field ids;
- field names;
- field types;
- base imports;
- access rules;
- typed traversal shape.

The block should read like a schema boundary for a native C++ type, not like a
serializer macro.

## What CONTRACT Is

- A small compile-time layer over native C++ structs.
- Stable external field ids and names.
- Typed field descriptors and traversal.
- Generated field descriptors that carry access shape.
- Base contract flattening through explicit offsets.
- Physical fields, reference fields, and logical/computed properties.
- Extension points for access customization, type codecs, and field policies.

## What CONTRACT Is Not

- Not runtime reflection.
- Not a generic serializer.
- Not a schema-first code generator.
- Not a runtime registry.
- Not a place for buffer management, SQL, protobuf, JSON, or vendor runtime code.

The contract layer should compile away. The selected adapter owns the remaining
runtime cost.

That claim is checked, not just asserted: measured against real libprotobuf
(3.21.12), CONTRACT's protobuf adapter packs faster in nearly every scenario
(typically 1.2x-3x) and unpacks faster in most (0.4x-0.9x), with byte-for-byte
identical wire output. See
[`adapters/protobuf.md#performance`](../adapters/protobuf.md#performance).

## Positioning Rule

`CONTRACT` should not compete head-on with serializers as a format library.

It should provide serializers and adapters with a compile-time contract and
leave wire format, transport, and runtime policy to the adapter family.
