# Compact Adapter

This page documents the native compact tagged binary adapter,
`contract::adapters::compact`. The adapter is not protobuf-compatible and does
not replace the existing [`binary`](binary.md) adapter.

## Purpose

`compact` sits between the existing binary and protobuf adapter families:

- `binary` is a raw positional adapter for fast native payloads;
- `protobuf` is a protobuf-compatible adapter with protobuf wire constraints;
- `compact` is a native self-describing binary format with stable field ids,
  cheap skipping, compact scalar encoding, and streaming-friendly object
  traversal.

## Design Goals

- Use stable `CONTRACT` field ids as wire field ids.
- Preserve forward compatibility for added fields by allowing unknown field
  values to be skipped.
- Preserve backward compatibility for missing fields by leaving target fields
  at their existing/default values.
- Encode common small values in one byte when possible.
- Encode integers using the minimum required payload width.
- Encode strings, byte arrays, arrays, maps, and objects with explicit sizes.
- Support one-pass read and write for normal object traversal.
- Avoid protobuf sizing prepasses for nested objects where the compact object
  representation can carry an item count instead of a byte size.
- Keep error handling aligned with the shared adapter diagnostic model.
- Keep the current `binary` adapter unchanged as the raw positional fast path.

## Non-Goals

- No protobuf wire compatibility.
- No runtime schema registry.
- No runtime reflection.
- No exception-based parser control flow.
- No PMap-style dynamic document model in the adapter core.
- No hidden per-field error bookkeeping on the hot path.
- No ABI/native-memory dump format.
- No reliance on host endian, host floating layout beyond explicitly specified
  wire choices.

## Reference Features To Preserve

The reference codec had several useful properties:

- a compact one-byte header for many scalar and container cases;
- dedicated encodings for zero, small unsigned integers, signed integers,
  strings, floats, arrays, maps, null, and bool;
- compact sizes embedded into container/string headers when they fit;
- recursive `skip()` for any value because each value carries its own shape;
- object fields written as `field-id, value` pairs;
- field ids may be emitted in a stable order to enable faster descriptor
  matching;
- a retained descriptor position while decoding, so the reader does not need to
  restart field matching from the beginning for ordered input;
- end-of-object cleanup that skips remaining unknown fields;
- distinction between inline/positional fields and indexed fields.

`compact` should keep these semantics but express them through the current
`CONTRACT` descriptor model, status/result flow, and adapter diagnostics.

The CONTRACT object wire shape is implemented as the object value codec.
`reader` and `writer` still own byte I/O and the intermediate
`read_field_value` / `write_field_value` dispatch points, so field-specific
adapter policy stays centralized without special-casing objects in
`read_value` / `write_value`.

Use [`contract::adapters::compact.hpp`](../../include/contract/adapters/compact.hpp)
for the core adapter surface and
[`contract::adapters::compact/all.hpp`](../../include/contract/adapters/compact/all.hpp)
for the full container codec set.

Accepted features for the compact adapter:

- self-describing value layer;
- compact scalar/header encoding with sign-magnitude signed integers;
- small size stored in the value header when possible;
- object fields encoded as `field-id, value`;
- descriptor-position fast path for writer-produced CONTRACT-order input;
- end-of-object cleanup that skips remaining unknown fields;
- recursive `skip_value()` as a required reader primitive;
- stream-to-stream `compare_value()` as a planned capability after the basic
  stream is proven.

Reference features that are intentionally not part of the base adapter:

- PMap/dynamic document storage;
- implicit scalar coercions;
- pointer/void payloads;
- exception-based parser flow;
- generated macro visitor infrastructure.

## Wire Model

The format is a stream of self-describing values:

```text
value := header + optional payload
```

Each header identifies the value kind and may also carry a small payload or a
small size.

Required value kinds:

```text
zero
small_uint
int
bytes
string
float32
float64
array
map
object
null
bool_false
bool_true
```

Accepted wire choices:

- `string` and raw `bytes` are distinct wire kinds.
- signed integers use sign-magnitude.
- `object` is a distinct wire kind, not a generic `map`.
- the first format version uses the one-byte header layout below.

The preferred direction is to define `object` explicitly instead of treating a
CONTRACT object as a generic map. That lets object decoding enforce field-id
rules and diagnostics without making every map behave like a schema object.

## Header Layout

The first compact format version uses one byte of header:

```text
0x00          zero

0x01..0x3f    small_uint 1..63

0x40..0x4f    small_neg -1..-16

0x50..0x5f    int payload
              low bits:
                bit 3      sign
                bits 0..2  byte_count_minus_1
              payload:
                magnitude, little-endian, 1..8 bytes

0x60..0x6f    bytes
              low 4 bits:
                0..14  inline byte size
                15     extended size follows as compact_uint

0x70..0x7f    string
              same size rule

0x80..0x8f    array
              low 4 bits:
                0..14  item count
                15     extended count follows as compact_uint

0x90..0x9f    map
              low 4 bits:
                0..14  pair count
                15     extended count follows as compact_uint

0xa0..0xaf    object
              low 4 bits:
                0..14  field count
                15     extended count follows as compact_uint

0xb0          bool_false
0xb1          bool_true
0xb2          null

0xc4          float32
0xc8          float64

0xd0..0xff    reserved
```

The layout is intentionally range-based:

- header decoding is mostly byte-range dispatch;
- `0`, small positive integers, and small negative integers do not need a
  payload;
- small byte sizes and item counts do not need a separate size value;
- large sizes and counts reuse the compact unsigned integer encoding;
- `object` and `map` remain distinct;
- the reserved range leaves room for future value kinds.

Examples:

```text
0                  -> 00
42                 -> 2a
-1                 -> 40
-16                -> 4f

string size 5      -> 75 + 5 bytes payload
string size 100    -> 7f + compact_uint(100) + payload

object 3 fields    -> a3
object 20 fields   -> af + compact_uint(20)
```

For integer payload headers:

```text
0x50..0x57 unsigned magnitude, 1..8 bytes
0x58..0x5f negative magnitude, 1..8 bytes
```

Examples:

```text
64       -> 50 40
255      -> 50 ff
256      -> 51 00 01
-17      -> 58 11
-256     -> 59 00 01
```

Canonical writer rules:

- `0` is encoded as `0x00`.
- `1..63` are encoded as `small_uint`.
- `-1..-16` are encoded as `small_neg`.
- integer payload headers are used only when the value does not fit a small
  integer form.
- extended size/count is used only when size/count is at least `15`.
- reserved headers are invalid on read until a future format version assigns
  them.

These rules keep the representation deterministic enough for future
`compare_value()` and encoded-byte equality.

## Integer Encoding

Compact integer encoding should optimize these cases:

- `0` is one byte;
- small positive integers are one byte;
- larger integers use the smallest number of payload bytes needed;
- negative integers are explicit and do not rely on native signed
  representation.

The reference codec used a one-byte zero, a small unsigned fast path, and a
sign bit with minimal payload bytes. `compact` keeps that size property and
specifies the byte order and signed representation explicitly.

Accepted rule:

```text
unsigned: minimal little-endian payload bytes
signed:   sign-magnitude, minimal little-endian magnitude payload bytes
```

Rationale:

- sign-magnitude is close to the reference codec and keeps `-1..-16` compact
  through the small negative header range;
- minimal unsigned bytes are easy to skip and bound-check;
- the wire representation is independent from host signed integer layout.

## Size Encoding

Sized values include:

- bytes/string;
- arrays;
- maps;
- objects.

For small sizes, the size may be embedded in the header. For larger sizes, the
header is followed by a compact unsigned integer size.

The size unit must be explicit:

- bytes/string size is bytes;
- array size is item count;
- map size is pair count;
- object size is field-pair count.

Object size as field-pair count is important because it allows one-pass object
writing without a nested byte-size prepass.

## Object Encoding

A `CONTRACT` object is encoded as:

```text
object_header(field_count)
repeated field_count times:
    compact_uint(field_id)
    value
```

Rules:

- `field_id` is the effective flattened `CONTRACT` field id.
- `field_id` must be positive.
- duplicate field ids are not detected as an error: the later occurrence on the
  wire overwrites the earlier one for that field. This is not enforced or
  tracked, it is simply the natural result of applying fields in wire order.
- writers emit fields in flattened `CONTRACT` descriptor order.
- readers decode field ids in any order (non-decreasing, decreasing, or
  otherwise) at the same cost; there is no writer-order fast path.
- unknown field ids are skipped with `skip_value()`.
- missing known fields are not errors by default; the target value keeps its
  existing/default state.
- field completeness is not checked: a reader never verifies that every known
  field id was present on the wire.

For each wire field id, the reader looks up the matching descriptor with
[`contract::dispatch_field_by_id<T>(id, fn)`](../../include/contract/visit.hpp)
- the same core primitive protobuf and yaml use for this, not a bespoke
per-adapter scan. It compares every descriptor's id against the wire id in
one fused pass (no hint, no per-object bookkeeping carried between fields):

```text
for each wire field:
    field_id, matched = false
    for each descriptor:
        if not matched and descriptor.id == field_id:
            decode descriptor value, matched = true
    if not matched:
        skip unknown field value
```

This avoids a runtime field registry, same as before, but no longer tries to
special-case writer-produced order: measurements showed the extra hint/wrap
bookkeeping cost more than it saved.

## Field Ordering

`compact` defines a stable writer field order:

```text
flattened CONTRACT descriptor order
```

Writer behavior:

- write fields in the flattened descriptor order produced by `CONTRACT`;
- omit fields only if an explicit adapter option later introduces default-value
  elision;
- otherwise write every visible field.

Reader behavior:

- scan descriptors from index 0 for every wire field, independent of writer
  order or any previous match;
- an id outside writer order costs the same as one inside it, so out-of-order
  input is valid without a runtime field registry and without a slower path;
- a repeated field id overwrites the previously decoded value for that field
  (last one wins); this is not treated as an error and is not detected.

Keeping stable CONTRACT writer order is useful for readability and forward
scans by other tools, but the reader does not rely on it for correctness or
performance.

## Unknown And Missing Fields

Unknown field:

```text
field id is present on the wire but not known by the current CONTRACT type
```

Default behavior:

- skip its value recursively;
- continue decoding;
- do not store unknown fields.

Missing field:

```text
field id is known by the CONTRACT type but absent on the wire
```

Default behavior:

- keep existing object value;
- if the caller constructed a fresh object, this means the field keeps its
  default-initialized value;
- required-field semantics, if added later, must be an explicit attribute or
  adapter option.

This is the main evolution advantage over the current raw binary adapter.

## Inline And Positional Values

The reference visitor distinguished indexed fields from inline/const fields.
The new adapter should keep the distinction conceptually:

- `CONTRACT` object fields are indexed by field id;
- plain values, array elements, tuple elements, variant payloads, and map
  keys/values are positional values inside their enclosing container.

No fake field id should be invented for plain nested payloads.

Tuple and variant mappings need adapter-specific conventions:

- tuple may encode as an array of positional values;
- variant should encode an explicit alternative index followed by the payload;
- variant payload is a positional value, not a CONTRACT field unless the
  alternative itself is a CONTRACT object.

## Containers

Array/vector:

```text
array_header(count)
repeated count times:
    value
```

Map:

```text
map_header(count)
repeated count times:
    key
    value
```

Rules:

- array size is item count;
- map size is pair count;
- map key ordering is not normalized unless a later option explicitly asks for
  canonical deterministic output;
- duplicate map keys follow the container policy. For `std::map`, inserting a
  duplicate key should be reported as a decode error unless we deliberately
  choose last-wins.

## Initial Type Mapping

The first implementation layer contains value primitives and `CONTRACT` object
traversal. Standard containers and optional/variant mappings are added after
the object stream is stable.

Initial mappings:

- `bool` uses dedicated `bool_false` / `bool_true` headers.
- unsigned integers use compact unsigned integer encoding.
- signed integers use compact signed sign-magnitude encoding.
- enums are encoded through their underlying integer type; reads decode the
  underlying integer and then cast to the enum type.
- `float` uses `float32` with little-endian IEEE-754 payload.
- `double` uses `float64` with little-endian IEEE-754 payload.
- `std::string` uses the `string` header and copies the payload into owned
  storage on read.
- `std::string_view` writes as `string` and reads through `peek(size)` /
  `consume(size)`, so it requires contiguous input lifetime.
- optional values use `null` for empty and the contained value otherwise.
- `std::array<T, N>` and raw C arrays (`T[N]`) use `array_header(N)` followed
  by positional element values and require the same count on read.
- `std::array<T, N>` and raw C arrays (`T[N]`) with byte-like `T` (e.g.
  `char`, `unsigned char`, `std::byte`) are the exception: they use
  `bytes_header(count)` with a single bulk copy instead, where `count` is
  `N * sizeof(T)` minus any trailing run of zero bytes. The writer trims a
  trailing zero run before writing, and the reader accepts
  `count <= N * sizeof(T)` and zero-fills the remaining tail. This is
  lossless for any content (the elided bytes are reconstructed as zero) and
  is not a C-string convention: there is no scan for an embedded NUL, only
  the literal tail of the buffer is examined.
- `std::vector<T>` uses `array_header(count)` followed by positional element
  values.
- `std::span<T>` uses `bytes_header(byte_count)` for byte-like spans and
  `array_header(count)` for typed spans.
- `std::bitset<N>` uses `bytes_header(ceil(N / 8))` with packed bits.
- `std::tuple<...>` uses `array_header(tuple_size)` followed by positional
  elements.
- `std::variant<...>` uses `array_header(2)` followed by `index, value`.
- `std::map<K, V>` and `std::unordered_map<K, V>` use
  `map_header(pair_count)` followed by positional key/value pairs.
- CONTRACT objects use `object_header(field_count)` followed by
  `compact_uint(field_id), value` pairs.

Planned mappings:


`field_id`, sizes, counts, enum payloads, and variant indexes all use the same
ordinary compact integer value encoding. There is no separate raw varint
encoding hidden inside the format.

## Skip Semantics

`skip_value()` is a core reader operation.

It must:

- read one header;
- skip scalar payload bytes directly;
- recursively skip array elements;
- recursively skip map key/value pairs;
- recursively skip object field id/value pairs;
- maintain depth and item limits;
- report truncation and invalid headers through compact diagnostics.

This operation is what makes unknown object fields safe.
The recursive implementation is owned by an internal `skip_reader`, while
`reader::skip_value()` remains the public entry point used by object decoding.

## Compare And Partial Inspection

The reference codec also supported stream-to-stream compare without
materializing the full value.

`compact` should support this as a planned capability after the base reader and
writer are stable:

```text
compare_value(lhs_reader, rhs_reader)
```

Potential uses:

- binary key comparison;
- sorted storage;
- testing deterministic encodings;
- fast equality checks for already encoded payloads.

This should not block the first minimal implementation, but it is part of the
adapter direction rather than an unrelated extension.

## I/O Model

`compact` should be streaming-first.

Reader:

- uses a window-style backend with `peek(size)` / `consume(size)`;
- exposes byte-count `read(out, size)` as a convenience layer for simple
  codecs;
- exposes `peek_byte()` / `consume_byte()` for header preview, primarily for
  `optional` and future tagged-shape codecs;
- allows codecs that benefit from contiguous payloads, such as
  `std::string_view`, to use `peek(size)` / `consume(size)` directly;
- does not require protobuf-style bounded nested windows to decode an object;
- must be able to skip unknown values from a stream.

Writer:

- writes headers and payloads sequentially;
- is window-backed and expects contiguous `prepare(size)` / `commit(size)`
  output access;
- uses the direct `prepare(size)` / `commit(size)` path as the hot write path
  for compact payloads;
- does not need to precompute nested object byte sizes.

This differs from protobuf, where length-delimited nested payloads often force
bounded nested windows or sizing prepasses. `compact` uses window mechanics for
cursor preview, contiguous payload access, and direct output writes.

## Diagnostics

`compact` must follow the shared adapter diagnostic model:

- hot-path operations return `read_status` / `write_status`;
- detailed error objects are created only on failure;
- no auxiliary state may be maintained only for diagnostics;
- leaf failures set the compact-specific code and stage;
- parent traversal may append type name, field descriptor, object stage,
  container index, or position correction;
- diagnostics are append-only by convention.

See [`rationale/errors.md`](../rationale/errors.md) for what a diagnostic
message is made of in general. In compact's case specifically:

```cpp
#include <contract/adapters/compact/all.hpp>
#include <contract/contract.hpp>
#include <contract/io.hpp>

#include <array>
#include <cstdint>
#include <string>

struct Customer {
    std::uint32_t id;
    std::string name;
    CONTRACT(Customer, (id, 1), (name, 2))
};

int main() {
    Customer original{7, "alice"};

    std::array<unsigned char, 64> buffer{};
    contract::adapters::compact::writer<> out(
        contract::io::window_output{buffer.data(), buffer.size()});
    out << original;
    const std::size_t written = out.position();

    // Truncate the last 2 bytes to simulate a short read.
    contract::adapters::compact::reader<> in(
        contract::io::window_input{buffer.data(), written - 2});
    Customer restored{};
    in >> restored; // throws
}
```

```text
compact reader: truncated while reading raw bytes in Customer field name (#2)
[member] at offset 5 (expected 5, got 3) [created at
contract/adapters/compact.hpp:728 in read_status
contract::adapters::compact::reader<>::read(void *, std::size_t)
[Input = contract::io::window_input]]
```

Here the field name, id, and kind are all known (the reader had already
matched the wire field id to a declared field before decoding its value
failed), unlike protobuf's unknown-field case.

Current read error codes:

```text
unknown
input_error
truncated
invalid_header
invalid_size
invalid_integer
invalid_field_id
duplicate_key
max_items_exceeded
span_size_mismatch
unsupported_operation
```

Current write error codes:

```text
unknown
output_error
invalid_size
invalid_field_id
max_items_exceeded
unsupported_operation
```

Planned read/write error codes for container codecs:

```text
max_depth_exceeded
max_bytes_exceeded
variant_index_out_of_range
```

Suggested stages:

```text
header
integer
size
raw_bytes
array
map
object
field_id
field_value
variant
span
skip
```

## Attribute Handling

Initial rules should mirror the binary adapter unless a compact-specific reason
exists:

- `check::max_length()` guards strings and byte-like values;
- `check::max_bytes()` guards byte payloads;
- `check::max_items()` guards arrays/maps/object item counts where applicable;
- `security::encrypt()` should not be copied blindly from binary until the
  byte-range boundaries for encrypted compact values are specified;
- `security::no_log()` remains out of scope for a binary wire adapter.

## Public Shape

Expected public types:

```cpp
contract::adapters::compact::writer<Output>
contract::adapters::compact::reader<Input>
contract::adapters::compact::options
contract::adapters::compact::codec<T>
```

Expected codec signatures:

```cpp
static write_status write(Writer& out, const T& value);
static write_status write(Writer& out, const Field& field, const T& value);
static read_status read(Reader& in, T& value);
static read_status read(Reader& in, const Field& field, T& value);
```

The field-aware path is where a codec may use the real `CONTRACT` field id,
name, kind, and attributes. The value-only path is for positional payloads.
`writer` is intended for window-backed outputs; the default output type is
`contract::io::window_output`.

## Implementation Strategy

Implemented base stream:

1. Header encode/decode.
2. Compact unsigned/signed integer encode/decode.
3. `skip_value()`.
4. Bytes/string.
5. Object write/read with CONTRACT-order writer order and a flat, order-independent
   per-field descriptor lookup on read (no hint, no runtime field registry).
6. Optional values as `null | value`.
7. `std::array<T, N>` and raw C arrays (`T[N]`) as `array_header(N)` plus
   positional values; byte-like `T` (e.g. `char`, `unsigned char`,
   `std::byte`) is the exception, using a single bulk `bytes_header(count)`
   copy instead, with trailing zero bytes trimmed on write and zero-filled
   on read.
8. `std::vector<T>` as `array_header(count)` plus positional values.
9. `std::span<T>` as `bytes_header(byte_count)` for byte-like spans and
   `array_header(count)` for typed spans.
10. `std::bitset<N>` as `bytes_header(ceil(N / 8))` with packed bits.
11. `std::tuple<...>` as `array_header(tuple_size)` plus positional values.
12. `std::variant<...>` as `array_header(2)` plus `index, value`.
13. `std::map<K, V>` and `std::unordered_map<K, V>` as `map_header(count)` plus
    positional key/value pairs.

Known limitations:

- No built-in byte-count or nesting-depth ceiling. A malformed size/count
  header cannot make the reader over-read the window (reads are bounds-checked
  and fail with a truncated error), but a large declared string/container size
  is reserved before the read fails, so an untrusted payload can request a
  large allocation. Bound this per field with `check::max_items` /
  `check::max_length` where inputs are untrusted; a global default ceiling is
  a candidate future addition.

Possible future capabilities:

- stream-to-stream `compare_value()`;
- optional deterministic map ordering, if a use case needs normalized bytes.

Out of scope:

- PMap/dynamic document values;
- default-value elision;
- encryption;
- cross-version schema negotiation.

## Compatibility Rules

Once published, a compact field id is stable.

Allowed changes:

- add a new field with a new field id;
- remove a field from the current C++ type while previous payloads carrying
  that field remain skippable;
- reorder C++ declaration only if changing byte-level writer order is
  acceptable for the use case; readers match by field id, but encoded bytes are
  deterministic in CONTRACT order.

Risky or incompatible changes:

- changing a field id;
- reusing a removed field id for a different meaning;
- changing a field's wire kind incompatibly;
- changing signed integer representation after data has been persisted;
- changing object field ordering semantics after readers depend on a specific
  unordered-input policy.

## Relationship To Existing Adapters

`compact` should not make `binary` more complicated.

Keep the split:

- `binary`: raw positional C++-native payloads;
- `compact`: tagged compact stream with skip/evolution;
- `protobuf`: protobuf-compatible interchange.

If `compact` succeeds, it becomes the preferred native persisted/transport
format when protobuf compatibility is not required.
