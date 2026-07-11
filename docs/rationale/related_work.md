# CONTRACT Related Work

This page keeps a short summary of the adjacent-library ideas that influenced
CONTRACT.

It is not a feature comparison matrix and not a canonical API doc. It records
the lessons that matter for the current design.

## What We Learned From Nearby Approaches

Different libraries stress different parts of the problem:

- reflection-heavy systems make discovery easy but can push behavior into
  runtime registries;
- serializer-first systems make wire shape easy but tend to duplicate schema
  knowledge across formats;
- lightweight descriptor systems make traversal cheap but need strong field
  metadata and policy boundaries;
- generator-driven systems make compatibility explicit but can be cumbersome
  when the source model already exists in C++.

The useful lesson is not that one family wins everywhere. The useful lesson is
that each family exposes a different tradeoff.

## What CONTRACT Reuses

From the surrounding ecosystem, CONTRACT keeps the parts that proved valuable:

- explicit field metadata;
- compile-time traversal shape;
- field-level customization;
- type-specific encoding hooks;
- schema and compatibility checks;
- small public query surface;
- separation between model, policy, and wire shape.

## What CONTRACT Does Not Copy

CONTRACT deliberately does not copy the parts that make other systems awkward
for this use case:

- runtime reflection as the primary model;
- a giant serializer DSL inside the data type;
- hidden global registries;
- format logic mixed into the core model;
- forced code generation when the source model already exists.

## Why Related Work Still Matters

Related work helps us decide when to use a direct adapter and when to use a
bridge:

- direct adapter when the format is native to the runtime;
- bridge or generator when another ecosystem owns the final wire contract;
- schema checker when the main value is compatibility validation;
- debug or audit adapter when the main value is inspection.

The point is to keep the model small while still respecting the ecosystem
around it.

## Design Rule

Use other libraries as design input, not as a template to copy.

The current contract tree should keep the lessons, not the legacy structure.
