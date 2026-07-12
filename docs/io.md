# CONTRACT I/O

This page describes the I/O-facing side of CONTRACT: byte-oriented backends
and thin convenience facades that adapters can build on top of.

## Responsibilities

- Own raw byte cursors and backing storage lifetime.
- Expose byte-oriented read/write primitives.
- Expose optional capabilities such as contiguous read views.
- Host convenience facades like [`contract::io::cout`](../include/contract/io/cout.hpp).
- Stay free of traversal, field metadata, and type-specific adapter policy.
- Reduce external sources and sinks to a small byte contract; do not grow
  protocol-specific semantics here.

## Backend Model

The I/O layer is intentionally small:

- `read(out, size)` copies bytes from the backend into caller storage;
- `write(data, size)` copies bytes from caller storage into the backend;
- `read_view(size)` is an optional contiguous-input capability, not a base
  requirement;
- `peek(size)` / `consume(size)` model a windowed readable region when a backend
  can expose contiguous unread bytes directly;
- the backend owns cursor movement, bounds checks, and backing storage
  lifetime.

The concrete backend types live in [`include/contract/io/byte.hpp`](../include/contract/io/byte.hpp):

- `output` / `checked_output` for byte sinks;
- `input` / `checked_input` for byte sources.

File-backed helpers live in [`include/contract/io/file.hpp`](../include/contract/io/file.hpp):

- `file_input` exposes streaming `read(...)`;
- `file_buffer_input` reads the whole file into owned memory and exposes
  `peek(...)` / `consume(...)` for adapters that require a windowed input.

The checked variants bound reads or writes to a known span and report failure
by returning a short/zero result (`write` returns the bytes written, `read`
and `read_view` signal truncation with `0` / `nullptr`); they do not throw.
The unchecked variants are the minimal fast path used by trusted inputs and
benchmarks.

The I/O layer does not define traversal or contract semantics. It only provides
the byte-level capability that adapters can build on top of.

## Windowed Read Backends

Some backends can expose a contiguous readable prefix without copying it out of
the storage object. That shape is modeled by `window_input`:

```cpp
class window_input {
public:
    std::span<const std::byte> peek(std::size_t max_size);
    void consume(std::size_t size);
};
```

Semantics:

- `peek(max_size)` returns up to `max_size` contiguous unread bytes from the
  current front;
- `peek()` does not consume bytes;
- `consume(size)` advances the front by `size` bytes;
- any previous span returned by `peek()` is invalid after `consume()`.

This is a consumer-side contract. It does not imply producer-side buffer
management.

The repository also provides an optional Beast-backed adapter for this shape in
[`include/contract/io/beast_window.hpp`](../include/contract/io/beast_window.hpp):
`flat_buffer_input` models the same `peek` / `consume` interface on top of
`boost::beast::flat_buffer` when Boost.Beast headers are available.

## Windowed Write Backends

Some backends can expose a writable prefix without copying data into an
intermediate sink first. That shape is modeled by `window_output`:

```cpp
class window_output {
public:
    std::span<std::byte> prepare(std::size_t max_size);
    void commit(std::size_t size);
};
```

Semantics:

- `prepare(max_size)` returns up to `max_size` contiguous writable bytes at the
  current front;
- `prepare()` does not commit bytes;
- `commit(size)` advances the front by `size` bytes;
- any previous span returned by `prepare()` is invalid after `commit()`;
- `prepare(max_size)` may return fewer bytes than requested when less writable
  space is available;
- `commit(size)` is a preconditioned cursor advance, not a checked write API.

This is a producer-side contract. It does not imply consumer-side finalization
or ownership transfer beyond the committed region.

When a backend can provide a contiguous writable window, adapters can build
encode paths that write directly into the destination buffer without going
through a second throwing sink abstraction.

For example, the protobuf adapter's varint encoder writes straight into the
window instead of building the value in a stack buffer first
([`include/contract/adapters/protobuf.hpp`](../include/contract/adapters/protobuf.hpp)):

```cpp
auto window = out_.prepare(10); // 10 bytes always fits any 64-bit varint
if (window.size() >= 10) {
    std::size_t count = 0;
    std::uint64_t v = value;
    while (v >= 0x80u) {
        window[count] = static_cast<std::byte>((v & 0x7fu) | 0x80u);
        ++count;
        v >>= 7;
    }
    window[count] = static_cast<std::byte>(v);
    ++count;
    out_.commit(count);
    return write_status::ok;
}
```

## Facades

[`contract::io::cout`](../include/contract/io/cout.hpp) is the neutral convenience facade used by adapter code
that wants a ready-to-use stream-like sink without console-specific presets.
It is a ready-to-use object, not a second adapter family.

[`contract/io.hpp`](../include/contract/io.hpp) is the external convenience
header for consumers that want the common lightweight I/O types in one place.
Inside this repository, prefer direct includes of the specific backend headers
instead of using the umbrella header.

Exception policy for the I/O layer is documented in
[`rationale/errors.md`](rationale/errors.md).

Additional facades may exist when they are thin and ergonomic, but they should
stay re-export-like and avoid becoming parallel public APIs.

The console-first facade lives at [`contract::cout`](../include/contract/cout.hpp) and is documented with the
adapter layer, not here.

## Public Surface

- [`contract::io::cout`](../include/contract/io/cout.hpp)
- backend `read` / `write` primitives
- optional `read_view(size)` capability for backends that can provide it
- optional `peek(size)` / `consume(size)` capability for contiguous windowed
  inputs
- optional `prepare(size)` / `commit(size)` capability for contiguous windowed
  outputs

## Related Docs

- [`adapters/README.md`](adapters/README.md)
- [`adapters/binary.md`](adapters/binary.md)
- [`reference/examples.md`](reference/examples.md)
- [`reference/benchmarks.md`](reference/benchmarks.md)
