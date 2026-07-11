# CONTRACT Exception Policy

This page defines the base exception policy for the CONTRACT layer.

The goal is to keep the hot data path predictable while allowing convenience
APIs and boundary checks to stay ergonomic.

## Base Rules

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

## Core

The core layer should remain compile-time driven.

- invalid contracts fail compilation;
- runtime validation is not part of the core;
- runtime registries are not part of the core;
- dynamic checks are not part of the core contract model;
- runtime exceptions should not define core invariants.

When the core needs to express a misuse of the static model, it should do so as
compile-time failure or as a contract rule, not as a runtime exception path.

## I/O

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

## Adapters

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

## Practical Guidance

- `file_input` open failure may throw.
- byte bounds wrappers may throw when they are explicit checked variants.
- fast readers should return status instead of throwing for malformed or short
  input.
- `consume()` should not be a checked read and should not return short counts.
- `need_more` should be a normal parser status, not an exception.

## Design Rule

Do not let exceptions define the normal protocol for the core, I/O, or
adapter hot path.

Use exceptions for boundary failures and user-facing convenience APIs.
Use status/result for incremental parsing, hot loops, and expected partial
progress.
