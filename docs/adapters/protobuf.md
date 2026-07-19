# Protobuf Adapter

This page describes the protobuf-compatible adapter family.

See [`adapters/README.md`](README.md) for the shared adapter contract and
execution split.

## Example - Nested Message, Wire Bytes, Round-Trip

```cpp
#include <contract/adapters/protobuf/all.hpp>
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

struct Order {
    std::uint64_t order_id;
    Customer customer;

    CONTRACT(Order, (order_id, 1), (customer, 2))
};

int main() {
    Order order{42, Customer{7, "kid"}};

    std::array<unsigned char, 64> buffer{};
    contract::adapters::protobuf::writer<> out(
        contract::io::window_output{buffer.data(), buffer.size()});
    out << order;

    // 08 2a           tag 1 (varint), value 42
    // 12 07           tag 2 (length-delimited), length 7
    //   08 07           nested Customer: tag 1 (varint), value 7
    //   12 03 6b 69 64  nested Customer: tag 2 (length-delimited), "kid"
    const std::size_t written = out.position();

    Order restored{};
    contract::adapters::protobuf::reader<> in(
        contract::io::window_input{buffer.data(), written});
    in >> restored;

    // restored.order_id == 42, restored.customer.name == "kid"
}
```

The wire bytes above are exactly what a `.proto`-generated `Order { uint64
order_id = 1; Customer customer = 2; }` message would produce - the same
schema declaration drives both the C++ struct's shape and the wire mapping,
with no separate `.proto` file or code generation step.

## Scope

The adapter encodes and decodes protobuf-compatible wire data for `CONTRACT`
models.

- `CONTRACT` field ids map to protobuf field numbers.
- The adapter follows the same `reader` / `writer` / `codec<T>` split as the
  binary adapter family.
- field-aware overloads parse protobuf tags, wire types, and length prefixes;
  value-only overloads consume only the payload and require an already bounded
  reader or writer. The field-aware path receives the real CONTRACT descriptor,
  so it can see field id, name, kind, and attributes.
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

See [`rationale/errors.md`](../rationale/errors.md) for what a diagnostic
message is made of in general. In protobuf's case specifically:

```cpp
// Hand-crafted wire bytes: field 1 (order_id, varint), then field 99, which
// has no matching CONTRACT field on Order (types as declared above).
std::vector<unsigned char> bytes{
    0x08, 0x2a,       // tag 1 (varint), value 42
    0x98, 0x06, 0x07, // tag 99 (varint), value 7
};

contract::adapters::protobuf::reader<> in(
    contract::io::window_input{bytes.data(), bytes.size()});
Order restored{};
in >> restored; // throws
```

```text
protobuf reader: unknown field while reading field key in Order at offset 4
(wire field #99) [created at contract/adapters/protobuf.hpp:375 in
read_status contract::adapters::protobuf::reader<>::read_message(T &)
[Input = contract::io::window_input, T = Order]]
```

An unknown field has no CONTRACT descriptor, so the message names only the
raw wire field number (`wire field #99`), not a field name or kind.

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
carry no protobuf dependency), built with Clang 19 (the `default` CMake
preset). GCC reproduces the same wire-format parity but shows a larger
`int25` unpack gap than Clang - see the compiler note in
[`reference/benchmarks.md`](../reference/benchmarks.md#reference-result-snapshot)
before treating either compiler's numbers as representative of the other.

See [`reference/benchmarks.md`](../reference/benchmarks.md#reference-result-snapshot)
for the current numbers (that page is the canonical source of measured
ratios - not restated here to avoid the two places drifting apart). In
short: wire sizes match byte-for-byte in every measured scenario, including
negative int32/int64 sign extension, and CONTRACT is faster in most pack and
unpack scenarios.

The two scenarios with a real, reproducible unpack slowdown are `int25`
(25 separate scalar `uint32_t` fields) and `vector[100]` (a 100-element
repeated scalar field) - both are shapes with many cheap elements and no
strings to mask per-element dispatch/decode cost. This traces to
`read_varint`'s per-element dispatch cost on that specific shape; two
alternative decode strategies (a branchless multi-byte SWAR decoder, and
mirroring libprotobuf's own single-byte fast path) were tried and both
measured worse on this codebase's actual value distributions - see the
benchmark's git history for the disassembly-backed reasoning. Nested
messages and string-heavy messages show no such gap.

Where the adapter wins, it wins for four concrete reasons rather than one:

- **Direct window writes, no scratch buffer.** `writer::write_varint_payload`
  writes varint bytes straight into the caller-owned
  `contract::io::window_output` via `prepare`/`commit`
  (`include/contract/io/byte_window.hpp`), instead of building the value in a
  stack buffer and `memcpy`-ing it out - a runtime-sized `memcpy` also
  defeats inlining, which the window path avoids entirely.
- **No upfront whole-message size pass for the value being serialized.**
  `write_message_by_index` writes fields directly as it walks the contract;
  there is no separate "measure the whole message, then serialize" pass for
  the value at the call site.
- **A dedicated sizer for the one place a size genuinely has to be known
  ahead of the bytes.** A length-delimited nested message still needs its
  byte length before its length prefix can be written. `measure_encoded_size`
  reuses the exact same `codec<T>::write` path as the real write, through a
  `counting_output` that only accumulates a position counter and skips
  varint byte construction entirely - so sizing a nested message costs a
  counting pass over the same code, not a scratch allocation or a second
  real serialization.
- **A fused-fold field dispatch instead of a per-field linear rescan.**
  Reading a message previously re-scanned the field list from index 0 for
  every wire field (quadratic in field count for ascending wire order);
  `dispatch_field_by_id` (`include/contract/visit.hpp`) finds the matching
  declared field and invokes the caller's handler in one fold expression, in
  a form the compiler can fold into a dense jump table the same way a
  literal `switch` would.

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
