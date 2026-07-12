# CONTRACT Errors And Exceptions

This page covers two related things: what an adapter's runtime diagnostic
looks like and how it's built (below), and the policy for when CONTRACT
throws versus returns a status/result (further down).

## Diagnostic Anatomy

Every adapter builds its runtime diagnostics on the same shared model
(`contract::detail::adapter_error_base` in
[`include/contract/detail/error.hpp`](../../include/contract/detail/error.hpp)).
A diagnostic accumulates, append-only, as it's passed up through nested
reads/writes, then renders to one human-readable message.

This model exists under a hard constraint: **it must cost nothing on the
success path**.

- the diagnostic object is created lazily, only on the first failure - the
  hot read/write loop never allocates or touches it while things are going
  well;
- it is never a separate bookkeeping structure kept "just in case" - it is
  built only from state the adapter already has for normal operation (the
  current field, the current offset, the current type), not from anything
  maintained solely for future error reporting;
- building and rendering the message itself is off the hot path entirely -
  it only happens once, after failure is already known, never per field or
  per byte.

See [Exception Policy](#exception-policy) below for the fuller rule this
follows from. Example (real, compiled output - truncating the last 2 bytes
of an otherwise valid encoding):

```text
compact reader: truncated while reading raw bytes in Customer field name (#2)
[member] at offset 5 (expected 5, got 3) [created at
contract/adapters/compact.hpp:728 in read_status
contract::adapters::compact::reader<>::read(void *, std::size_t)
[Input = contract::io::window_input]]
```

Reading left to right:

- `compact reader` - which adapter and side (reader/writer) produced the
  diagnostic;
- `truncated` - the error code (an adapter-specific enum);
- `while reading raw bytes` - the action and the stage it failed at;
- `in Customer field name (#2) [member]` - the CONTRACT type, then the field
  name, declared id, and access kind, attached together when a field
  descriptor is known at the point of failure;
- `at offset 5` - the byte offset in the input/output where the failure was
  detected, when the I/O backend tracks position;
- `(expected 5, got 3)` - a size mismatch attached by the adapter, when
  relevant;
- `[created at ...]` - a developer trace: the exact source file, line, and
  instantiated function signature that created the diagnostic object, so a
  developer can jump straight to the failing code path.

Not every piece appears in every message - each is attached only when it is
actually known at the point of failure:

- an *unknown* field has no CONTRACT descriptor, so its message has no field
  name or kind, only the raw wire id;
- the yaml adapter's `missing_required_key` check does not attach a field to
  the diagnostic today, so that message does not name the missing key in its
  human-readable part - only the developer trace (via the instantiated
  `Field = ...` template argument) identifies it.

Each adapter's own docs page has one more worked example in its own wire
format:
[`adapters/protobuf.md`](../adapters/protobuf.md#scope),
[`adapters/compact.md`](../adapters/compact.md#diagnostics),
[`adapters/yaml.md`](../adapters/yaml.md#error-model).

## Exception Policy

The goal is to keep the hot data path predictable while allowing convenience
APIs and boundary checks to stay ergonomic.

### Base Rules

Exceptions are allowed when the failure is outside the normal data path.

Exceptions are not allowed when the state is part of ordinary parsing,
incremental I/O, cursor movement, or other expected forward progress.

If the caller is expected to continue normal work after the condition, the
condition should be represented as a status or result.

The usual split is:

1. fast primitives
   - `noexcept` or effectively no-throw;
   - preconditions;
   - `assert` in debug builds for caller bugs.

2. core parser / encoder loops
   - status or result values;
   - no exceptions for routine control flow.

3. public convenience APIs
   - may throw when they wrap the core with a simpler user-facing contract.

### Core

The core layer should remain compile-time driven.

- invalid contracts fail compilation;
- runtime validation is not part of the core;
- runtime registries are not part of the core;
- dynamic checks are not part of the core contract model;
- runtime exceptions should not define core invariants.

When the core needs to express a misuse of the static model, it should do so as
compile-time failure or as a contract rule, not as a runtime exception path.

### I/O

The I/O layer may throw for boundary and setup failures.

- constructor and setup failures;
- file open failures;
- invalid public API arguments in checked or convenience wrappers;
- optional convenience wrappers that trade strictness for simplicity.

The checked byte-span backends themselves do not throw on truncation or
overflow: they return a short/zero result and let the caller decide.

The I/O layer should not use exceptions for normal cursor movement or partial
progress.

For windowed readers, the contract is:

```text
peek()      -> provide a contiguous unread window
codec       -> analyze the window
consume(n)  -> confirm how many bytes were actually used
```

`consume(n)` is a preconditioned cursor advance, not a partial read:

- precondition: `n <= available()`;
- effect: advance the cursor by `n`;
- violation: bug in the caller, not malformed input.

`consume()` should not become a partial read API.

### Adapters

Adapters own runtime parsing and encoding behavior.

- parser and encoder hot loops should use status/result control flow;
- diagnostic objects, when used, must be derived from the existing runtime state
  that already exists for normal adapter behavior; do not maintain auxiliary
  state only for future error reporting;
- a diagnostic object may keep an internal developer trace such as
  `file:line in function signature` for the place where it was created, and
  that trace may be copied when the diagnostic is transferred or wrapped;
- `need_more`, `would_block`, `partial frame`, and similar states should be
  normal statuses, not exceptions;
- leaf helpers may throw for rare boundary failures, but they should not turn
  exceptions into the normal parse protocol;
- convenience wrappers may throw when they present an all-or-nothing contract
  to the caller;
- checked adapters may throw when they intentionally choose a strict API.

For maximum throughput:

- keep `throw` out of inner loops;
- keep checked throwing wrappers separate from the fast path;
- prefer `assert` for caller bugs in debug builds.

For example, the protobuf adapter's status-based `write_value(...)` is the hot
path; `operator<<` is the thin convenience wrapper around it that throws
([`include/contract/adapters/protobuf.hpp`](../../include/contract/adapters/protobuf.hpp)):

```cpp
template<class T>
writer& operator<<(const T& value) {
    using value_type = contract::adapters::base::clean_t<T>;
    error_.reset();
    const auto status = write_value(value);
    if (status == write_status::error) {
        if constexpr (contract::adapters::base::has_contract_definition<value_type>) {
            error().type_name(contract::type_name<value_type>())
                .stage(detail::write_stage::message_root);
        }
        throw std::runtime_error(error_message());
    }
    // ... (compile-time check that value_type is a CONTRACT type, elided)
    return *this;
}
```

Nothing inside `write_value(...)` itself throws for a malformed or
insufficient buffer - it returns `write_status::error` and lets the caller
decide. Only this outer, optional convenience boundary turns that into an
exception.

### Practical Guidance

- `file_input` open failure may throw.
- byte bounds wrappers may throw when they are explicit checked variants.
- fast readers should return status instead of throwing for malformed or short
  input.
- `consume()` should not be a checked read and should not return short counts.
- `need_more` should be a normal parser status, not an exception.

### Design Rule

Do not let exceptions define the normal protocol for the core, I/O, or
adapter hot path.

Use exceptions for boundary failures and user-facing convenience APIs.
Use status/result for incremental parsing, hot loops, and expected partial
progress.
