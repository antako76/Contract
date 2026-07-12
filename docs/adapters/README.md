# Adapter Layer

[`contract::adapters`](../../include/contract/adapters/README.md) owns the runtime behavior that turns contract metadata
into a concrete representation.

Use this page when you design or review any adapter family. It describes the
shared boundary model; the family-specific page should only describe what is
different.

Exception and status/result policy is documented in
[`../rationale/errors.md`](../rationale/errors.md).

## Why this layer exists

The core contract layer defines the `CONTRACT` object model:
fields, ids, names, access semantics, and compile-time traversal shape.

See [`../core.md`](../core.md) for the class-level model, and [`../../include/contract/contract.hpp`](../../include/contract/contract.hpp) for the corresponding header.

The adapter layer defines behavior:

- binary encoding;
- console/debug rendering;
- logging and audit output;
- file-backed or stream-backed export;
- transport adapters for network or IPC delivery.

This split keeps contract metadata stable while allowing each adapter family to
choose its own runtime policy without polluting the core model.

As a rule, external protocols are adapted at the boundary into the shared byte
and traversal contract; the protocol itself should not leak deeper into the
adapter layer than necessary.

## Adapter Template

Before adding a new adapter family, answer these questions:

- what backend it consumes or produces;
- what the user-facing entry point is;
- what state, if any, has to survive nested traversal;
- what belongs in `codec<T>` and what belongs in the reader/writer;
- what helper code is reusable and what is just one-off forwarding.

If these answers are unclear, the adapter contract is not ready yet.

## Execution Split

The common adapter execution path is intentionally small:

```text
contract::field descriptors
  Field::kind            member / reference / property classification
  Field::get(obj)        adapter-facing read access
  Field::ref(obj)        direct physical storage reference when available
  Field::set(obj, value) semantic write path for properties and custom setters

contract::io
  raw byte cursor, bounds checks, and storage lifetime
  input.read(out, size)
  input.read_view(size)
  output.write(data, size)
  output.prepare(size) / output.commit(size)

writer / reader
  descriptor traversal
  direct-ref vs temporary-value policy
  typed dispatch entry points

codec<T>
  wire format for one value type
  value composition through typed dispatch
```

The rule is:

- `contract` knows field identity, kind, and access shape;
- `io` knows bytes and cursor movement;
- `io` may expose optional contiguous read or write windows when an adapter
  can use them directly;
- `writer` / `reader` know traversal and dispatch;
- `codec<T>` knows the concrete shape of one value type when the adapter
  chooses to factor that shape out.
- adapter traits own the handling matrix, and the contract dispatch point
  validates the current `T` against it.
- field-aware code uses the descriptor's `kind`, `get`, `ref`, and `set`
  functions instead of inventing a second access model.
- helper functions should stay small and local; if they only move data between
  existing methods, they are usually a sign that the boundary is still blurry.

The call chain has two distinct forms:

```text
1. user -> adapter                # user calls the adapter
2a. adapter -> direct primitive    # simple values may be handled directly
2b. adapter -> field-aware codec    # CONTRACT fields may use a codec with the full field contract
3. adapter dispatch -> nested values
4. return to adapter
```

- direct primitive handling is only for simple values;
- field-aware codec use is only for real CONTRACT fields;
- nested values recurse through the adapter dispatch, not by direct codec-to-
  codec calls.

## State Boundaries

Adapters may own runtime/session state, but only when the state is necessary
for the adapter contract.

- keep configuration separate from runtime/session state;
- store traversal state in the owning reader or writer, not in leaf helpers;
- if a value has to survive nested traversal, the adapter owns that lifetime;
- if code must guess whether it is root or nested, the state contract needs to
  be clarified before more code is added.

## Failure and Diagnostic Model

Adapters are expected to use status/result control flow in the hot path and
keep detailed diagnostics on the failure path.

- normal parse/encode flow should return a status or result, not throw;
- diagnostic objects should be created only when a failure occurs;
- diagnostic data must be derived from existing runtime/session state at the
  point of failure; the hot path must not maintain auxiliary error-only state;
- the diagnostic object owns the final failure report for that event;
- diagnostics may carry an internal developer trace such as
  `file:line in function signature` for the point where the diagnostic object
  was created; this trace is copied with the diagnostic, appears at the end of
  the message, and is not the user-facing failure contract;
- the shared adapter error interface follows the same shape across readers and
  writers:
  - `const std::optional<error_type>& error() const noexcept` exposes the
    retained diagnostic after a failure;
  - `error_type& error(const error_type& child) noexcept` transfers a child
    diagnostic into the current object so parent layers can append context;
  - private `error(std::source_location location = std::source_location::current())`
    creates the local diagnostic lazily on the first failure;
  - the first failing operation sets the concrete adapter code and stage;
  - later layers may only append missing enclosing context, such as the field,
    type, path, stage, position correction, or adapter entry point;
- leaf helpers should record only facts they know locally at the point of
  failure;
- parent layers may add only context that the child cannot know, such as the
  surrounding field or adapter entry point;
- when a child diagnostic is transferred to a parent, the parent may correct a
  position or offset only if it knows the exact coordinate relationship between
  the nested and outer views;
- parent layers must not overwrite already recorded leaf facts;
- diagnostic setters should be idempotent and append-only by convention;
- a leaf should record the most specific facts it can observe locally, such as
  adapter-specific codes, offsets, positions, line/column, field identity,
  wire/token state, or expected/actual values when those are directly visible;
- a parent may only add enclosing context, such as the outer field/path/stage,
  position correction, or the adapter entry point that re-raises the failure;
- once a diagnostic field is populated, later layers may only fill an empty
  field or add a new field; they must not rewrite an existing fact;
- if a field descriptor is available, pass it into the diagnostic path so the
  adapter can reuse contract metadata in logs and error messages;
- shared diagnostics may include an additional locator such as a stage when the
  adapter cannot express the same distinction cleanly through error codes
  without making the code set explode.

## Shared Public Contract

This page is the single source of truth for adapter-facing public APIs.
If an adapter surface changes, update this section first and then update the
adapter-specific design notes to reference it.

### Shared Roles

#### `contract`

- Owns contract metadata, field descriptors, and compile-time traversal.
- Defines what a field is, how it is named, and what access capabilities it has.
- Does not own adapter formatting policy or runtime render/layout state.

#### [`contract::io`](../../include/contract/io.hpp)

- Owns backend-facing I/O primitives and convenience facades.
- May also expose optional contiguous windowed byte views for buffered backends.
- Hosts top-level ready-to-use instances such as [`contract::io::cout`](../../include/contract/io/cout.hpp).
- Does not own contract metadata or type-specific formatting policy.

#### [`contract::adapters`](../../include/contract/adapters/README.md)

- Owns adapter traversal plus rendering/encoding policy.
- Contains the execution engine for a concrete adapter family.
- May own internal per-run session state when the adapter needs a prepass or
  other computed layout.

#### `codec<T>`

- Owns the shape of one value type inside a concrete adapter family.
- Knows how one type is rendered or encoded.
- May also receive the full field contract on field-aware paths, so a codec
  can use the field id, name, and attributes when the adapter routes a real
  CONTRACT field through it.
- Does not own orchestration, session lifecycle, or top-level facade policy.

#### `writer` / `reader`

- Public adapter objects.
- They are the primary entry points for adapter-specific object-style APIs.
- They execute traversal and formatting/encoding against a configured backend.
- They may hold or reference internal per-run state when required by the adapter.

#### `session`

- Internal runtime state for one render/run.
- Used only for computed data that must survive nested traversal.
- Examples: comment layout, cursors, depth-related render state.

#### `facade`

- Thin convenience layer, usually in `contract::io` or a root alias header.
- Example: [`contract::io::cout`](../../include/contract/io/cout.hpp) and [`contract::cout`](../../include/contract/cout.hpp).
- Must stay a thin re-export or default instance, not a second implementation path.

### Shared Entry Rules

- `with(options)` is the universal configuration override hook.
- Adapter-specific presets such as `schema()`, `value()`, and `debug()` are
  optional sugar.
- `operator<<` / `operator>>` may serve as convenience boundaries for
  stream-like adapters, but the underlying adapter entry points should still be
  able to expose status/result flow directly.
- Public helpers like `dump(...)` or proxy wrappers should not be added if the
  object API already expresses the need.
- If an adapter needs a prepass, keep that orchestration internal and run it once
  per top-level entry.
- If a codec needs a sizing prepass, keep that helper internal to the adapter
  family rather than exposing a second public encode path.
- Prefer the existing object-style API over adding a second public wrapper
  around the same action.

## Field, Codec, and Traversal Rules

These rules describe how adapter code should be organized when a codec exists.

- `attributes` live on CONTRACT fields, not on raw values.
- field policy is checked at the field dispatch point, where the descriptor and
  the full field contract are visible.
- `codec<T>` is part of the adapter implementation, not a separate subsystem.
- a codec may exist only for the types that need it; simple types may be
  written or read directly by the adapter.
- when a codec is used for a CONTRACT field, it receives the field contract so
  it can see the field id, name, and attributes.
- a codec may also expose a value-only payload helper for a simple C++ type;
  that helper is for non-field payloads and does not receive a field contract.
- field-aware and value-only paths are distinct by design: a CONTRACT field can
  carry attributes, while a plain value cannot.
- value-only payload helpers are optional convenience functions for payload
  organization, not a second architecture layer.
- codec recursion should use the adapter's normal recursive dispatch for nested
  values, not direct codec-to-codec calls.
- a container codec should treat nested `T` as a value to be dispatched, even
  when `T` is itself a CONTRACT type.

Examples:

```text
field(name, "contract")
  -> field attributes exist here
  -> adapter may enforce max_length or other field rules
  -> if the adapter delegates to a codec, it passes the full field contract
  -> the codec can use field.id, field.name, field.attributes, and similar
     metadata to shape the wire form

vector<string>
  -> the vector codec may serialize each element as a value
  -> element strings do not get field attributes unless the adapter has a
     separate element-attribute model
  -> the vector codec must not call another codec directly by name; it dispatches
     each element through the adapter

vector<InnerContract>
  -> the vector codec writes elements as values
  -> each InnerContract element re-enters the adapter through normal contract
     traversal
  -> the element contract gets its own field descriptors when traversal reaches
     it; the vector codec itself does not invent a fake child field
```

Rule of thumb:

- field contract is for policy and field-shaped wire decisions;
- plain values do not carry field policy;
- value-only helpers are for payload organization;
- nested values recurse through the adapter, not through direct codec chaining.

### Canonical Field / Value Split

Use this as the working rule when you implement or review an adapter:

1. `field(...)` is the field entry point for both writer and reader.
2. `field(...)` is the only place where the adapter can see the full field contract.
3. If the field is a nested `CONTRACT` value, traversal re-enters the adapter.
4. If the field is a leaf value, the adapter may route it through
   `codec<T>::write/read(out, field, value)`. An adapter may define the
   field-aware overload as optional and fall back to
   `codec<T>::write/read(out, value)`.
5. If the value is a plain non-field payload, the adapter may use `codec<T>::write/read(out, value)` or a direct primitive path.
6. A codec may use the field contract only when the adapter passes a real field
   descriptor. When field-aware dispatch is optional, a codec adds that
   overload only when it uses the field contract.
7. One codec must not call another codec directly by name; recursion goes through adapter dispatch.

Examples:

```text
CONTRACT field
  -> field descriptor is visible
  -> adapter can enforce field policy
  -> codec may shape the wire form using field id/name/attributes

plain string payload
  -> no field contract
  -> adapter may use a value-only helper or direct write path

vector<InnerContract>
  -> vector codec handles the container payload
  -> each InnerContract element re-enters adapter traversal
  -> the vector codec does not invent a fake child field
```

In real code, the binary adapter's `std::string` codec shows both overloads
side by side
([`include/contract/adapters/binary.hpp`](../../include/contract/adapters/binary.hpp)):

```cpp
template<>
struct codec<std::string, void> {
    // value-only: no field contract, used for non-field payloads
    // (e.g. a vector<std::string> element)
    template<class Writer>
    static write_status write(Writer& out, const std::string& value) {
        const std::size_t size = value.size();
        if (out.write_value(size) == write_status::error) {
            return write_status::error;
        }
        if (value.empty()) {
            return write_status::ok;
        }
        return out.write(value.data(), size);
    }

    // field-aware: receives the full field contract, so it can enforce
    // field-level attributes (max_length/max_bytes here) before falling
    // through to the value-only overload above
    template<class Writer, class Field>
    static write_status write(Writer& out, const Field& field, const std::string& value) {
        if (const auto limit = attributes::max_length_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_length_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        if (const auto limit = attributes::max_bytes_limit(field); limit && value.size() > *limit) {
            return out.error()
                .code(write_error_code::max_bytes_exceeded)
                .field(field)
                .stage(write_stage::attribute_guard)
                .sizes(*limit, value.size());
        }
        return write(out, value);
    }
};
```

### Facade Defaults

- [`contract::io::cout`](../../include/contract/io/cout.hpp) is the neutral convenience facade for stream-like I/O
use.
- [`contract::cout`](../../include/contract/cout.hpp) is the console-first preset facade: it starts from the
  schema/debug view and enables the human-friendly console defaults used by
  examples and interactive inspection.

## What Stays Here

Adapter-specific notes in this directory should explain:

- what the adapter family is for;
- what its execution model looks like;
- which state it needs and why;
- which type capabilities it requires;
- which parts are public contract and which are implementation detail.

They should not redefine the public API surface independently.

## Adapter Responsibilities

In practice, each adapter family follows the same split:

- `writer` / `reader` execute traversal and rendering or encoding;
- `codec<T>` defines the type-specific shape inside that adapter family;
- `session` carries one-run state when the adapter needs a prepass or layout;
- facades in [`contract::io`](../../include/contract/io.hpp) or root aliases stay thin and convenience-only.

## Family Pages

- [Console Adapter](console.md)
- [Binary Adapter](binary.md)
- [Compact Adapter](compact.md)
- [JSON Adapter](json.md)
- [Protobuf Adapter](protobuf.md)
- [YAML Adapter](yaml.md)

## Rules

- Keep traversal and adapter policy together.
- Keep per-run state internal to the adapter.
- Keep type-specific rendering or encoding in `codec<T>`.
- Keep convenience facades thin.
- If a public adapter contract changes, update this page first.
