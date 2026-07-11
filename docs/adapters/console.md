# Console Adapter

This page is the adapter-specific entry for console debug rendering.
The shared public contract is in [`README.md`](README.md), and the implementation is split between [`../../include/contract/adapters/console.hpp`](../../include/contract/adapters/console.hpp) and [`../../include/contract/adapters/console/all.hpp`](../../include/contract/adapters/console/all.hpp).

## Implementation Note

This page is the current reference for console rendering behavior.

## Public Surface

- [`contract::adapters::console::writer<Output>`](../../include/contract/adapters/console.hpp)
- `writer.with(options)`
- `writer.schema()`
- `writer.debug()`
- `writer.value()`
- `writer << value`
- [`contract::io::cout`](../../include/contract/io/cout.hpp)
- [`contract::cout`](../../include/contract/cout.hpp) as the console-first preset facade

`debug()` is the richer audit preset: it keeps the schema view, enables the
console defaults, and adds extra provenance such as flattened base provenance
in field comments.

The lean `console.hpp` keeps the writer core, scalar/string codecs, and shared
layout helpers. Container codecs live in the dedicated headers under
[`../../include/contract/adapters/console/`](../../include/contract/adapters/console/),
and [`../../include/contract/adapters/console/all.hpp`](../../include/contract/adapters/console/all.hpp)
includes the full set.

## Rendering Policy

The console adapter splits responsibilities between `codec<T>`, `writer`, and
the stateless helpers in [`../../include/contract/adapters/debug/format.hpp`](../../include/contract/adapters/debug/format.hpp).

- `codec<T>` owns the local render recipe for a type:
  - it chooses `block` or inline rendering;
  - it chooses the render primitive for the type;
  - it adds type-specific comment metadata;
  - it may traverse children in the order that makes sense for that type.
- `writer` owns the shared rendering canvas:
  - indentation;
  - comments, alignment, and color;
  - `max_depth`, `max_items`, `max_string_length`, and `max_byte_preview_length`;
  - YAML-style syntax and line structure;
  - the common indexed-item and map-entry frames;
  - low-level leaf formatting primitives for scalars, strings, enums, bitsets, and byte blobs.
- `debug/format` helpers provide pure formatting only:
  - no indentation;
  - no writer state;
  - no layout policy;
  - just string and byte preview formatting utilities.

This keeps the common tree scaffolding centralized in the writer, while still
allowing complex codecs such as `optional`, `tuple`, `map`, `variant`, or
nested objects to control their own local layout without forcing an extra
schema-conversion layer.

## Purpose

The console adapter should make a contract inspectable by eye.

It should show:

- field id;
- field name;
- value type;
- storage type when it differs from the value type;
- member, property, base, and accessor origin;
- BASE offset and effective field id;
- nested contract object shape;
- container size;
- element indexes;
- truncation markers for long containers, strings, and byte blobs;
- whether the adapter used custom get/set/ref paths.

The output is a YAML-style object tree with CONTRACT metadata in comments.
It does not promise strict YAML compatibility.

## Output Shape

Top-level contract objects are printed as named YAML-style nodes.

```text
RequestEvent:
  id: 42                                  # #1 u32
  route: "/api/payments/{id}"             # #2 std::string_view
  tags:                                   # #3 std::vector<std::string>, size=2
    - "payment"                           # [0]
    - "critical"                          # [1]
  meta:                                   # #4 RoutedEvent
    RoutedEvent:
      service: "payment-api"              # #1 std::string
      operation: "payment.create"         # #2 std::string
```

This output is optimized for reading and golden tests, not for parsing back.

## Field Comments

Every field in `schema` mode gets a metadata comment.

Minimum comment:

```text
# #<field_id> <value_type>
```

Examples:

```text
id: 42                                   # #1 u32
display_name: "Ilya"                     # #8 std::string, property
route: "/api/payments/{id}"              # #2 value=std::string_view, storage=char[64]
count: 42                                # #101 u64, UserProfile+20
name: "checkout"                         # #12 std::string_view, storage=char[64], custom_get
```

Comment construction should be centralized in [`contract::adapters::debug`](../../include/contract/adapters/debug/), not
copied through printer branches.

## Type Names

The console adapter depends on stable, readable type names.

Required core support:

```cpp
[`contract::type_name<T>()`](../../include/contract/definition.hpp) -> std::string_view
```

For contract types, the name should come from the contract macro through
stringification:

```cpp
CONTRACT(RequestEvent, ...)
```

should preserve `"RequestEvent"` as the public contract type name.

[`contract::adapters::debug`](../../include/contract/adapters/debug/) should provide best-effort names for built-in and
common library types:

```text
bool
u8, u16, u32, u64
i8, i16, i32, i64
float, double
std::string
std::string_view
std::vector<T>
std::array<T,N>
std::bitset<N>
std::optional<T>
std::map<K,V>
std::unordered_map<K,V>
std::tuple<T...>
std::variant<T...>
```

Fallback compiler-specific names are acceptable only as a last resort.

## Containers

Sequences are printed as YAML-style lists.

Complex elements use nested nodes:

```text
events: # #5 std::vector<RequestEvent>, size=2
  - # [0]
    RequestEvent:
      id: 42                              # #1 u32
      route: "/api/payments/{id}"         # #2 std::string_view
  - # [1]
    RequestEvent:
      id: 43                              # #1 u32
      route: "/api/refunds/{id}"          # #2 std::string_view
```

Tuples use the same ordered list shape, with fixed arity and index comments for
each element:

```text
position: # #1 std::tuple<std::string,u32>, size=2
  - "left" # [0] std::string
  - 4 # [1] u32
```

Variants render the active alternative through the same indexed-item frame:

```text
status: # #2 std::variant<std::string,UserProfile>, size=2
  - "queued" # [0] std::string
```

Bitsets render as quoted bit strings with the fixed bit count in the schema
comment:

```text
permissions: "1011001010110010..." # #9 std::bitset<256>, set=37, truncated
```

Long containers use head-only truncation in P0:

```text
tags: # #3 std::vector<std::string>, size=128
  - "payment"                           # [0]
  - "critical"                          # [1]
  - "checkout"                          # [2]
  - ... # truncated, +125 items
```

Map formatting depends on key complexity.

Simple printable keys may use mapping form. In `schema` mode the entries still
keep indexed comments.

```text
headers: # #5 std::map<std::string,std::string>, size=2
  "content-type": "application/json"  # [0]
  "x-request-id": "abc"               # [1]
```

Keys with YAML-significant characters should be quoted:

```text
headers:
  "x-request/id": "abc"
```

[`std::unordered_map`](../../include/contract/adapters/debug/type_name.hpp)
uses the same rendering rules, but the output follows native iteration order
and is not sorted.

```text
labels: # #7 std::unordered_map<std::string,std::string>, size=1
  "priority": "high" # [0]
```

Complex keys use a sequence of entries:

```text
routes:                               # #6 std::map<RouteKey,RoutedEvent>, size=1
  - RouteKey:                         # [0] key
      method: "POST"                  # #1 std::string
      path: "/api/payments/{id}"      # #2 std::string
    RoutedEvent:                      # [0] value
      service: "payment-api"          # #1 std::string
      operation: "payment.create"     # #2 std::string
```

The important rule is that the codec decides the shape of the composite node:

- `sequence` renders ordered children with shared indexed-item framing;
- `tuple` renders ordered children with the same frame, and may attach
  per-element type annotations in schema mode;
- `map` decides whether a key can be rendered in mapping form or needs a
  `key`/`value` entry pair;
- `variant` renders the active alternative according to the currently held
  type, using the active alternative index in the indexed-item frame;
- `optional` renders either `nullopt` or the nested child;
- the writer only supplies the common tree scaffolding, indentation, comment
  layout, and low-level leaf formatting primitives;
- the codec decides the traversal order and composes those primitives into the
  local node shape for its type.

## Strings And Bytes

Strings are always quoted and escaped.

Required escapes:

```text
"
\
\n
\r
\t
control characters
```

Long strings use truncation when they exceed `max_string_length`.

Binary or byte-buffer values use either a text preview or a hex preview,
depending on content, not on the element type: a trailing run of zero bytes
is trimmed first, and if what remains is all printable ASCII, it prints as a
quoted string; otherwise the full, untrimmed buffer prints as hex. This is a
content decision, not a C-string convention - a single non-printable byte
anywhere (including one embedded before the end) falls back to hex of
everything, rather than guessing where real content stops.

```text
code:     "contract"                # #1 char[8], bytes=8
padded:   "hi"                      # #2 char[16], bytes=16
embedded: "61 62 63 00 64 65 66 00" # #3 char[8], bytes=8
```

`code` is fully packed text, so nothing is trimmed. `padded` has 14
trailing zero bytes trimmed before the printable check. `embedded` has a
trailing zero trimmed too, but the remaining bytes still contain an
embedded NUL, so the printable check fails and the full 8-byte buffer
prints as hex instead of a misleadingly truncated string.

A genuinely binary field still falls back to hex as before:

```text
permissions: "10 2b 9a ..." # #8 std::array<std::byte,256>, bytes=256, truncated
```

Byte blobs such as [`std::array<std::byte, N>`](../../include/contract/adapters/console.hpp),
[`std::array<unsigned char, N>`](../../include/contract/adapters/console.hpp),
[`std::array<char, N>`](../../include/contract/adapters/console.hpp), raw
byte/char arrays, and byte vectors are all eligible for the text preview.
The hex preview length follows the writer's `max_byte_preview_length`
option; the text preview follows `max_string_length`, the same limit used
for regular string fields.

## Optional

`std::optional<T>` prints as either `nullopt` or the nested value.

```text
user_id: nullopt # #10 std::optional<u64>
user_id: 42      # #10 std::optional<u64>
```

Complex optional values use nested nodes.

## Provenance

In `debug()` mode, imported base fields may show provenance like
`UserProfile+20`.

That mode is intended for audit/debug inspection, not for the compact schema
view.

## Determinism and Color

The console adapter should be deterministic by default so it can be used for
golden tests.

Color is optional and should stay disabled unless the preset enables it.

The console-first facade may enable color and comment alignment by default, but
the raw object API must still allow a neutral configuration.
