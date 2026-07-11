# CONTRACT Tradeoffs

This page explains the design choices behind the current CONTRACT split.

## Layer Split

The core layer should stay small:

```text
field id;
field name;
field type;
typed field reference;
base flattening;
property fields;
traits / customization points;
contract checks.
```

The core should not own:

```text
serialization format;
buffer management;
runtime schema registry;
heap allocations;
runtime reflection;
virtual dispatch;
vendor runtime.
```

Adapters own runtime behavior:

```text
protobuf wire adapter;
.proto generator/checker;
FlatBuffers schema/builder bridge;
Alpaca/custom binary adapter;
Glaze/JSON/MsgPack bridge;
debug dump adapter;
backup/restore adapter;
log/redaction adapter;
visualization adapter;
export adapter.
```

## Object-Based Public API

The adapter-facing public API should be object-based:

- `writer` / `reader` are public adapter objects;
- `with(options)` is the universal override hook;
- `schema()`, `value()`, `debug()` are optional presets;
- `session` is internal runtime state when the adapter needs a prepass or
  layout;
- convenience facades must stay thin and must not become a second
  implementation path;
- `dump(...)` should not become a parallel API if the object-based entry point
  already exists.

This keeps the external shape consistent across adapter families.

## Why Presets Are Optional

Not every adapter needs named presets.

- console/debug adapters benefit from `schema()`, `value()`, `debug()`;
- binary adapters may only need `with(options)`;
- other adapters may choose different presets or none at all.

The universal part is configuration override through `with(options)`.

## Why State Stays Internal

Some adapters need a one-time prepass or computed layout state.

That state should live inside the adapter execution path, not in the public API.
The reason is practical:

- the external API stays small;
- nested traversal can reuse the same run state;
- orchestration stays in one place;
- the adapter can keep the fast path when no state is needed.

## Why [`contract::cout`](../../include/contract/cout.hpp) Is a Facade

[`contract::io::cout`](../../include/contract/io/cout.hpp) is the neutral convenience facade.
[`contract::cout`](../../include/contract/cout.hpp) is the console-first preset facade.

The difference is in defaults, not in a second implementation path.
That gives us a human-friendly default for console output while keeping the raw
I/O layer neutral.

## Design Rules

- Keep traversal and adapter policy together.
- Keep per-run state internal to the adapter.
- Keep type-specific rendering or encoding in `codec<T>`.
- Keep convenience facades thin.
- Keep one public contract per adapter family.
- Keep benchmark methodology separate from the public API docs.
