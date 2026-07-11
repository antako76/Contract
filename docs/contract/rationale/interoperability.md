# CONTRACT Interoperability

This page explains how CONTRACT fits into existing serializer ecosystems and
code-generation flows.

It is not the public API contract. It describes integration paths, compatibility
levels, and bridge strategies that matter once a contract already exists.

## What Interoperability Means Here

`CONTRACT` is useful even when the final format is owned by something else.
The contract layer can feed:

- a direct runtime adapter;
- a schema generator;
- a schema checker;
- a compatibility bridge;
- a vendor-specific integration layer;
- a migration tool from handwritten serializers.

The main point is that the field graph is declared once and then reused.

## Compatibility Levels

Not every integration promise has the same strength.

Common levels:

- source compatibility: the declared contract can be consumed by a tool or
  adapter without changing the source model;
- schema compatibility: the declared field ids and shapes can be compared
  across versions;
- wire compatibility: the produced payload stays readable by older or newer
  consumers;
- behavioral compatibility: a bridge preserves the semantic meaning of the
  fields even if the wire shape changes.

Different adapters may choose different levels. The important part is to make
the level explicit instead of assuming that every format has the same rules.

## Direct Adapter vs Bridge

There are two broad integration styles:

1. Direct adapter
   - the adapter walks CONTRACT descriptors directly;
   - the adapter owns the runtime behavior;
   - best for debug output, binary encoding, and local format rules.

2. Bridge or generator
   - CONTRACT is used to generate or validate another schema or runtime layer;
   - best for protobuf-like writers, `.proto` export, builder/checker flows, or
     vendor-specific integration points;
   - the generated layer may own the final wire shape.

The project should support both styles without pretending they are the same
thing.

## Protobuf and Similar Formats

Protocol-buffer-like systems are a good example of the bridge case.

The useful CONTRACT role is not to replace protobuf semantics. The useful role
is to provide:

- stable field ids;
- explicit field names;
- explicit field kinds;
- base flattening rules;
- access rules;
- compatibility checks;
- generator input for a `.proto`-like or schema-like tool.

That can support:

- direct protobuf-style adapters;
- schema export;
- schema validation;
- version skew checks;
- migration reports.

## Vendor Bridge Strategy

Some ecosystems need a vendor-specific bridge rather than a direct adapter.

That bridge should stay outside the core model and live in an adapter or
tooling layer. Typical cases:

- mapping into an existing serializer runtime;
- adapting to a company-internal schema compiler;
- translating CONTRACT into a storage or messaging DSL;
- preserving a legacy wire format while the source model becomes declarative.

The contract layer should make this easier, not absorb the vendor runtime.

## Why "Keep Your Serializers" Still Matters

CONTRACT is not a universal serializer replacement.

If a project already has a stable serializer stack, CONTRACT can still help as:

- the shared schema declaration for the model;
- the input for adapter generation;
- the basis for schema checks and migration tools;
- the source for debug, binary, export, or audit adapters.

That is a stronger position than "rewrite all serializers in one more DSL".

## Compatibility Roadmap

The compatibility story should be explicit and incremental:

- start with the declared field graph and direct adapters;
- add schema checking and compatibility diagnostics;
- add generated bridge paths only where they are worth the cost;
- keep wire compatibility promises separate from source compatibility promises.

## Design Rules

- Keep interoperability concerns out of the core model.
- Keep bridge logic out of the public contract docs.
- Make compatibility levels explicit in docs and tooling.
- Prefer direct adapters when the format is native to the runtime.
- Prefer bridges when the format is owned by another ecosystem.
