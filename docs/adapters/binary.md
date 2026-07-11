# Binary Adapter

This page is the adapter-specific entry for the binary reader/writer pair.
The shared public contract is in [`README.md`](README.md). The lean core
implementation lives in
[`../../include/contract/adapters/binary.hpp`](../../include/contract/adapters/binary.hpp),
and the standard container families are split into opt-in headers under
[`../../include/contract/adapters/binary/`](../../include/contract/adapters/binary/).
Field/codec organization rules are described in [`README.md`](README.md).
Exception and status/result policy is documented in
[`../rationale/exceptions.md`](../rationale/exceptions.md).

## Public Surface

- [`contract::adapters::binary::writer<Output>`](../../include/contract/adapters/binary.hpp)
- [`contract::adapters::binary::reader<Input>`](../../include/contract/adapters/binary.hpp)
- `writer.with(options)` when adapter-specific options exist
- `writer << value`
- `reader >> value`

## Error Flow

The binary adapter follows the same diagnostic model as the protobuf and YAML
adapters:

- runtime read/write/field/codec operations return `read_status` or
  `write_status`;
- the binary adapter uses byte-count `io` backends on the raw read/write path;
- `reader` and `writer` retain a lazy `read_error` / `write_error` object after
  a failed status;
- the retained diagnostic interface mirrors the protobuf adapter:
  `error() const` exposes the stored diagnostic, `error(child)` transfers a
  nested child diagnostic into the current object, and the local diagnostic is
  created lazily on the first failure;
- public `operator<<` and `operator>>` are convenience boundaries that convert
  a failed status into `std::runtime_error` with the retained diagnostic
  message;
- hot-path code must not maintain extra state only for future diagnostics;
- the first failing operation sets the concrete error code and stage;
- parent traversal layers add type, field, container index, and stage context
  while the failed status unwinds;
- `contract::check::max_length()`, `contract::check::max_bytes()`, and
  `contract::check::max_items()` failures are reported through binary error
  codes, not by throwing from attribute guard helpers;
- duplicate map keys, invalid variant indexes, span size mismatches, truncated
  input, and output backend failures are reported through the retained binary
  error object.

The expected codec signatures are:

```cpp
static write_status write(Writer& out, const T& value);
static write_status write(Writer& out, const Field& field, const T& value);
static read_status read(Reader& in, T& value);
static read_status read(Reader& in, const Field& field, T& value);
```

## Guarantees

The binary adapter is a native binary adapter. It is not a portable
cross-platform wire format by default.

Current guarantees:

- no runtime reflection;
- no dynamic field registry;
- no per-field virtual dispatch;
- descriptor traversal is compile-time typed;
- direct physical fields are loaded through `ref(obj)` when the descriptor
  capability and the object category make it safe;
- properties and custom setters always go through `set(obj, value)`;
- composite values are built by adapter dispatch over nested values, while
  codecs only own the local payload shape of the type they implement;
- zero-copy read is available only when the input backend provides
  `read_view(size)`;
- the adapter does not invent a hidden object slot between descriptor and
  value.

Attribute handling:

- the binary adapter validates each `CONTRACT` type at its dispatch point with
  `contract::require_adapter_mode<T, contract::adapters::binary::adapter_traits>()`;
- `contract::check::max_length()`, `contract::check::max_bytes()`, and
  `contract::check::max_items()` are enforced as read-side decode guards;
- other `contract::check::*` attrs are ignored metadata;
- `contract::security::sensitive()` and `contract::security::secret()` are
  ignored by the binary codec;
- `contract::security::no_log()` is out of scope for binary;
- `contract::security::encrypt()` is enforced as a simple stream-level XOR
  obfuscation via `options::encrypt_key`;
- this is transport obfuscation, not cryptographic encryption.

Current non-goals:

- stable cross-compiler ABI format;
- endian-independent encoding;
- schema evolution;
- automatic ownership for raw pointers;
- transparent decoding of unsafe pointer fields.

## Dispatch Model

Value write:

```text
out << value
  -> if value is a contract object:
         traverse fields
     else:
         write the payload directly or through a value helper
```

Value read:

```text
in >> value
  -> if value is a contract object:
         traverse fields
     else:
         read the payload directly or through a value helper
```

Field write:

```text
writer.field(descriptor, obj)
  -> if the field is a simple value:
         adapter writes it directly or through a payload helper
     else if the field value is a CONTRACT type:
         adapter re-enters normal contract traversal
     else:
         adapter uses the value codec for the payload shape
```

Field read:

```text
reader.field(descriptor, obj)
  -> if direct field ref is allowed:
         read into the live field storage
     else:
         value_type value
         read into a temporary value
         descriptor.set(obj, value)
```

This is the preferred rule of thumb:

- `writer.field` and `reader.field` are the field-aware entry points;
- `writer << value` and `reader >> value` are the plain-value entry points;
- field-aware and plain-value paths are distinct by design;
- simple values are handled directly by the adapter on the plain-value path;
- if a type needs a special wire shape, that belongs in `codec<T>`;
- if the adapter uses a codec for a CONTRACT field, the codec receives the full
  field contract and can use the field id, name, and attributes;
- nested CONTRACT values re-enter the adapter through normal contract
  traversal, not by a direct codec-to-codec call; see [`README.md`](README.md)
  for the adapter-level field/codec split rules.

The canonical split is:

- `field(...)` is the adapter entry point for a real CONTRACT field;
- `codec<T>::write/read(out, field, value)` is the field-aware path for leaf
  values when the adapter wants the codec to shape the wire form;
- `codec<T>::write/read(out, value)` is the plain-value helper path for non-
  field payloads;
- nested `CONTRACT` values recurse through adapter dispatch, not codec-to-
  codec chaining.

## Binary Checklist

Use this list when you implement or review binary behavior:

- validate every `CONTRACT` type at the binary dispatch point with
  `contract::require_adapter_mode<T, adapter_traits>()`;
- keep field policy in `writer.field(...)` and `reader.field(...)`, not in leaf
  writers;
- enforce `check::max_length`, `check::max_bytes`, and `check::max_items` as
  field-level decode guards;
- handle simple values directly when that is clearer than introducing a codec;
- use `codec<T>` only for the payload shape of a specific type;
- if a codec has a field-aware overload, pass the full field contract to it;
- otherwise fall back to the value-only payload helper path;
- recurse into nested `CONTRACT` values through normal adapter dispatch;
- do not call one codec directly from another codec.

## IO Capability Split

The adapter is built on top of a small raw input/output protocol:

```text
input.read(out, size)   -> copy bytes and advance
output.write(data, size)
```

The adapter owns typed dispatch and field traversal. The input/output backend
owns cursor movement, bounds checks, and storage lifetime.

`read_view(size)` is an optional capability for contiguous input backends.
It enables zero-copy reads for codecs that can consume a contiguous byte span
directly. Stream, socket, and pipe inputs may implement only `read(out, size)`
and still satisfy the base input contract.

Buffered backends may also model a windowed consumer interface with
`peek(size)` / `consume(size)`. That shape is useful when the backend already
owns a contiguous readable prefix and the adapter can borrow it directly
without copying.

The output side is intentionally simpler: it only needs `write(data, size)`.
An output backend can expose the current cursor for sizing and
benchmarking.

## Zero-Copy Capability

Direct field access and contiguous input access are the same optimization idea
on different sides of the pipe:

```text
descriptor.ref(obj)   -> write directly into live object storage
input.read_view(size) -> read directly from contiguous input storage
```

Both are optional fast paths. The base contract remains value-oriented and
works without either capability:

```text
object write/read can always fall back to value materialization
```

When both capabilities are available, codecs and field traversal can avoid an
intermediate value copy. When one side is missing, the adapter falls back to
materializing a value and then assigning or setting it.

`std::string` and `std::string_view` intentionally model different ownership:

```text
std::string      owning; works with any input; copies bytes into string storage
std::string_view borrowed; requires read_view(size); points into input storage
```

For owning movable values such as `std::string` and `std::vector<T>`, semantic
setters are move-aware when deserialization performance matters. Prefer
accepting `T` by value or `T&&`. Accepting only `const T&` is valid, but can add
an extra byte copy during deserialization fallback.

## Implementation Patterns

The binary adapter intentionally prefers a small set of patterns:

- fixed-shape value types use typed element-wise dispatch, for example
  `std::tuple<T...>` and `std::array<T, N>`;
- contiguous payload blocks use raw byte writes only for payload bytes, not for
  the whole type graph;
- scalar-like primitives use the raw native codec path;
- borrowed views such as `std::string_view` require `read_view(size)`;
- `const char*` is write-only and is treated as a borrowed C-string payload,
  not as an owned string type;
- `std::vector<T>` uses typed length-prefix dispatch and may choose bulk copy
  for scalar-like `T`;
- `std::variant<T...>` stores the active alternative index followed by the
  payload of that alternative;
- `std::bitset<N>` is a fixed-size codec, with a small-N scalar fast path and a
  portable packed-bit fallback.
