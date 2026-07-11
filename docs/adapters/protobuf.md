# Protobuf Adapter

This page describes the protobuf-compatible adapter family.

See [`adapters/README.md`](README.md) for the shared adapter contract and
execution split.

## Scope

The adapter encodes and decodes protobuf-compatible wire data for `CONTRACT`
models.

- `CONTRACT` field ids map to protobuf field numbers.
- The adapter follows the same `reader` / `writer` / `codec<T>` split as the
  binary adapter family.
- field-aware overloads parse protobuf tags, wire types, and length prefixes;
  value-only overloads consume only the payload and require an already bounded
  reader or writer.
  For example:

  ```cpp
  static read_status read(Reader& in, const Field& field, detail::wire_type wire, T& value)
  static read_status read(Reader& in, T& value)
  static write_status write(Writer& out, const Field& field, const T& value)
  static write_status write(Writer& out, const T& value)
  ```
- Nested `CONTRACT` values map to nested messages.
- `std::array<T, N>` and raw C arrays (`T[N]`) represent a single packed
  repeated block with exactly `N` scalar-like elements.
- `std::array<T, N>` and raw C arrays (`T[N]`) with byte-like `T` (`char`,
  `unsigned char`, `std::byte`) are the exception: they map to a
  length-delimited `bytes` field instead, the same wire shape as
  `std::string`, with a trailing run of zero bytes trimmed on write and
  zero-filled on read (lossless, not a C-string convention).
- `std::vector<T>` represents a repeated field.
- `std::bitset<N>` represents a fixed-size `bytes` field with little-endian bit
  packing inside each byte.
- `std::map<K, V>` and `std::unordered_map<K, V>` represent repeated
  protobuf map-entry messages.
- `std::tuple<T...>` represents a positional nested message whose elements are
  encoded as fields `1..N`; field order on the wire is ignored, but every
  positional field must appear exactly once.
- `std::variant<...>` maps to a nested message with an explicit alternative
  index and payload field.
- nested `CONTRACT` payloads must be decoded from a bounded reader/window:
  `reader::read_message(...)` consumes the full current reader span, so callers
  must create a nested length-limited input before decoding embedded messages.
- `reader::read_message(...)` and `writer::write_value(...)` / field-aware
  codec dispatch use status/result flow internally.
- `reader` and `writer` keep detailed failure data in `reader::error()` /
  `writer::error()` as optional diagnostic objects that are created only on the
  failure path; `error(child)` transfers a nested child diagnostic into the
  current object, and the private builder form creates the local diagnostic
  lazily on first failure.
- `operator>>` / `operator<<` are convenience boundaries that translate a
  failed status into a thrown diagnostic message, while the hot path itself
  stays status-based.
- `read_error` keeps the protobuf wire field number separate from CONTRACT
  field metadata so the log context can show both the protocol failure and the
  descriptor-level field identity.
- protobuf adapters follow the shared adapter failure model:
  leaf parsing records local failure facts, parent layers add only missing
  context, and the final error object remains append-only by convention.
- when a nested message failure is transferred to the parent reader, the parent
  applies position correction only when it knows the exact relationship
  between the nested payload window and the outer reader position.
- internal developer trace is appended at the end of the message; a typical
  shape is `...[created at contract/adapters/protobuf/vector.hpp:74 in
  contract::adapters::protobuf::codec<std::vector<T>, void>::read]`.
- additional locators such as `stage` are acceptable only when they add real
  diagnostic value that would otherwise require a disproportionate explosion of
  error codes.
- nested message encoding uses an internal sizing prepass and then writes the
  payload directly to the destination output, without an intermediate buffer.

## Wire Rules

- unknown fields are errors;
- unknown enum values are errors;
- repeated scalar fields are read from both packed and unpacked wire forms;
- repeated scalar fields are written packed by default;
- map iteration order is not normalized;
- `reader` consumes a contiguous windowed byte backend through
  `peek()` / `consume()` and does not fall back to generic `read()` I/O;
- `writer` uses either generic `write()` I/O or contiguous `prepare()` /
  `commit()` windows depending on the backend capability;
- repeated scalar packing is controlled by `protobuf::options`;
- error reporting uses CONTRACT metadata and field metadata, but only on the
  failure path;
- `string` maps to owning text storage;
- `bytes` maps to owning byte storage such as `std::vector<std::byte>`.

## C++ Mapping

- scalar fields use the same supported type set as the binary adapter family;
- `std::array<T, N>` and raw C arrays (`T[N]`) represent a fixed-size
  repeated field backed by one packed length-delimited block;
- `std::array<T, N>` and raw C arrays (`T[N]`) with byte-like `T` are the
  exception: they map to `bytes` instead (trailing zero bytes trimmed on
  write, zero-filled on read);
- `std::optional<T>` represents an optional field;
- `std::vector<T>` represents a repeated field;
- `std::bitset<N>` maps to `bytes` with a fixed payload length of `ceil(N / 8)`
  bytes and LSB-first bit packing;
- `std::map<K, V>` and `std::unordered_map<K, V>` represent repeated
  map-entry messages with `last wins` on duplicate keys;
- `std::variant<...>` represents a nested message with an explicit variant
  index and payload;
- enums are validated against known values;
- packed repeated applies only to scalar-like repeated fields and is optional
  via adapter options.

## Protocol Constraints

- length-delimited fields carry an explicit byte length;
- nested messages are encoded as length-delimited payloads;
- packed repeated scalar fields are encoded as a length-delimited block;
- `oneof` is a distinct protobuf field-group construct, not a general
  container type;
- unknown fields are part of the wire format, not the value model;
- borrowed views require a stable contiguous byte span.

## Key Tradeoffs

- length-delimited encoding requires the payload size before the final write;
- packed repeated encoding requires the packed block size before the final write;
- nested messages add a wrapper length around every nested payload;
- the reader path is simpler with contiguous windows than with arbitrary
  streaming input;
- container-like mappings such as `std::vector` and `std::variant` need
  adapter-defined conventions on top of the protobuf wire format.

In practice, this means protobuf is a reasonable interchange layer for
well-bounded message shapes, but it is a poor fit when the caller wants to:

- stream large payloads without a sizing pass;
- parse from small windows while the full object is still incomplete;
- treat container shapes as first-class wire primitives;
- avoid extra wrapper lengths and repeated passes over the data.

## Performance

Measured against real libprotobuf (3.21.12) with an optional benchmark
(`benchmarks/protobuf_reference_benchmark.cpp`, gated behind
`CONTRACT_BENCH_WITH_PROTOBUF`, off by default so the core library and tests
carry no protobuf dependency).

- Wire sizes match byte-for-byte in every measured scenario, including
  negative int32/int64 sign extension - this is a real interop check, not
  just a size estimate.
- Pack is faster than libprotobuf in nearly every scenario, typically by
  1.2x-3x (e.g. a single string field: ~0.3x of libprotobuf's time).
- Unpack is faster in most scenarios (~0.4x-0.9x), with one known exception:
  messages with many individual scalar integer fields of the same type (e.g.
  25 separate `uint32_t` fields) unpack ~1.1-1.3x slower. This traces to
  `read_varint`'s per-field dispatch cost on that specific shape; two
  alternative decode strategies (a branchless multi-byte SWAR decoder, and
  mirroring libprotobuf's own single-byte fast path) were tried and both
  measured worse on this codebase's actual value distributions - see the
  benchmark's git history for the disassembly-backed reasoning.
- Repeated scalar fields, nested messages, and string-heavy messages show no
  such gap - the effect is narrow to that field shape, not general.

Run it yourself:

```sh
cmake -S . -B build -DCONTRACT_BENCH_WITH_PROTOBUF=ON
cmake --build build --target contract_protobuf_reference_benchmark
./build/benchmarks/contract_protobuf_reference_benchmark --iterations 200000
```

## Implementation Limits

- nested `CONTRACT` values are written as length-delimited messages;
- packed repeated scalar fields are emitted as one packed block;
- the reader uses a contiguous windowed backend for payload parsing;
- `std::variant<...>` is encoded as an adapter-defined nested message with an
  explicit index and payload field;
- unknown fields and unknown enum values are treated as errors in the default
  contract;
- nested payload errors are reattached to the outer error context with a
  position correction.
- embedded `CONTRACT` values are always read through a nested bounded reader
  created from the enclosing length-delimited payload.

## Non-Goals

- no forward-compat skip mode for unknown fields in the default contract;
- no custom extension registry in the initial contract;
- no sorting for map emission;
- no protobuf reader/writer API that deviates from the shared adapter shape.
