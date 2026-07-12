# JSON Adapter

The JSON adapter is the structured writer for CONTRACT values. It is
writer-only in this repository; there is no JSON reader.

See [`adapters/README.md`](README.md) for the shared adapter contract and
execution split.

## How To Use

The simplest entry point is `to_string(value, options)`:

```cpp
#include <contract/adapters/json/all.hpp>
#include <iostream>

const auto text = contract::adapters::json::to_string(event);

contract::adapters::json::options opt;
opt.secret = contract::adapters::json::security_mode::redact;
const auto redacted = contract::adapters::json::to_string(event, opt);
```

If you want a stream-style writer, use `writer<Output>` directly:

```cpp
contract::adapters::json::writer<std::ostream&> out(std::cout);
out << event;
```

This is the intended usage shape:

- define a `CONTRACT` type next to the data model;
- keep the object as the source of truth;
- choose `to_string(...)` for a ready-made JSON string;
- choose `writer<Output>` when you want to write to an existing sink;
- pass `options` when you need to control security handling.

## What It Writes

The adapter writes structured data to JSON text:

- contract objects become JSON objects;
- scalars become JSON scalars;
- `std::optional<T>` becomes either `null` or the nested value;
- `std::vector<T>` becomes a JSON array;
- `std::array<T, N>` and raw C arrays (`T[N]`) become a JSON array of `N`
  elements;
- `std::array<char, N>` and raw `char[N]` are the exception: they become a
  single JSON string, with a trailing run of zero bytes trimmed first. This
  is a type decision (`char` is text), not a content scan, since JSON is
  primarily used for the hot-path logger; `signed char`/`unsigned char`/
  `std::byte` stay numbers, matching any other numeric array;
- `std::tuple<T...>` becomes a JSON array;
- `std::variant<T...>` becomes `[index, value]`;
- `std::map<K, V>` and `std::unordered_map<K, V>` become arrays of `[key, value]` pairs;
- `std::bitset<N>` becomes a quoted bit string;
- strings are escaped as JSON string literals, using `\uXXXX` for control
  characters outside the named escapes (`\n`, `\r`, `\t`, `"`, `\\`) - this
  differs from the console adapter's `\xNN` debug convention, which is not
  valid JSON.

## Security Options

The JSON writer can hide or redact fields based on contract attributes:

- `no_log`
- `secret`
- `sensitive`

Each attribute is controlled by a matching `security_mode` entry:

- `ignore` keeps the value;
- `omit` removes the field from output;
- `redact` emits `"<redacted>"`.

This is useful for structured logs and audit output where the schema stays
visible but sensitive values must not leak.

## Example

The repository test case [`tests/unit/adapters/json/json_adapter_test.cpp`](../../tests/unit/adapters/json/json_adapter_test.cpp)
shows:

- nested contract objects;
- security redaction;
- `std::optional`;
- vectors, arrays, tuples, variants;
- ordered and unordered maps;
- `std::bitset`;
- field-aware vs value-only codec dispatch.

## Structured Logging

`contract::logging` is a separate layer that builds newline-delimited JSON
records on top of this writer. The logger owns the event envelope; JSON owns
the serialization.

The public example is [`examples/logging.cpp`](../../examples/logging.cpp).
It combines a context object, a structured body, and typed attributes:

```cpp
RequestContext context{};
context.service = "billing";
context.request_id = 42;

Payment payment{};
payment.order_id = 17;
payment.currency = "USD";

log.with(context).info(
    "payment_completed",
    contract::logging::format("Captured payment for order {} in {} ms", payment.order_id, 15),
    contract::logging::attribute("payment", payment),
    contract::logging::attribute("duration_ms", std::uint32_t{15}));
```

That call becomes one JSON record:

```json
{"timestamp":"2026-06-14T12:34:56.789Z","severity":"info","severity_number":9,"name":"payment_completed","body":"Captured payment for order 17 in 15 ms","context":{"service":"billing","request_id":42},"attributes":[{"name":"payment","value":{"order_id":17,"currency":"USD"}},{"name":"duration_ms","value":15}]}
```

When redaction is enabled for security-sensitive fields, the same flow keeps
the record shape and replaces sensitive values with `"<redacted>"`.

## How It Works

- `writer` owns the JSON output state and comma/brace layout.
- `codec<T>` owns the type-specific JSON shape.
- field-aware dispatch is used when the writer is walking a real CONTRACT field.
- value-only dispatch is used for plain payloads and helper conversions.
- `options` owns the security policy for contract attributes.

## Public Surface

- [`contract::adapters::json::writer`](../../include/contract/adapters/json.hpp)
- [`contract::adapters::json::options`](../../include/contract/adapters/json.hpp)
- [`contract::adapters::json::security_mode`](../../include/contract/adapters/json.hpp)
- [`contract::adapters::json::to_string`](../../include/contract/adapters/json.hpp)
- [`contract/adapters/json/all.hpp`](../../include/contract/adapters/json/all.hpp)
