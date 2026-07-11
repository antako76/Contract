# Logging

`contract::logging` builds structured log events on top of the JSON adapter.
It is a separate layer: CONTRACT describes values, JSON serializes them, and
the logger owns event metadata and record boundaries.

For the JSON serialization side, see [`adapters/json.md`](adapters/json.md).
For a runnable end-to-end example, see [`examples/logging.cpp`](../../examples/logging.cpp).

```cpp
#include <contract/logging.hpp>

contract::logging::logger log{output};

log.with(context).info(
    "payment_completed",
    "Captured payment for order 17",
    contract::logging::attribute("order_id", order_id),
    contract::logging::attribute("duration_ms", duration_ms));
```

Each call writes one newline-delimited JSON record:

```json
{"timestamp":"2026-06-14T12:34:56.789Z","severity":"info","severity_number":9,"name":"payment_completed","body":"Captured payment for order 17","context":{},"attributes":[{"name":"order_id","value":42},{"name":"duration_ms","value":15}]}
```

## Contract

- Levels are `trace`, `debug`, `info`, `warning`, `error`, and `critical`.
- The JSON record includes `severity` and `severity_number`.
- The logger creates a UTC RFC 3339 timestamp with millisecond precision.
- `with(context)` keeps a non-owning reference to a context value.
- Without `with(context)`, the record contains an empty context object.
- The event identifier is serialized as `name`.
- `body` stores the human-readable message when it adds value.
- `attribute(name, value)` creates a non-owning attribute entry.
- `attr(name, value)` is a short alias for `attribute(name, value)`.
- Serialization is synchronous; referenced values must remain alive until the
  logging call returns.
- Every record ends with `\n`.
- Logging functions are `noexcept`. Output and serialization failures are
  ignored unless an `options::on_error` handler is configured.
