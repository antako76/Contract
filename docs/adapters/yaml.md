# YAML Adapter

The YAML adapter is a strict streaming config reader built on top of the
shared adapter contract model.

See [`adapters/README.md`](README.md) for the shared adapter contract and
execution split.

## How To Use

The adapter reads structured configuration data into an already declared C++
object.

```cpp
#include <contract/adapters/yaml/all.hpp>
#include <contract/cout.hpp>
#include <contract/io.hpp>

struct PaymentConfig {
    std::string service;
    std::uint32_t port = 0;
    bool enabled = false;
    std::vector<std::string> tags;

    CONTRACT(PaymentConfig,
        (service, 1),
        (port, 2),
        (enabled, 3),
        (tags, 4))
};

contract::adapters::yaml::reader<contract::io::file_buffer_input> in(
    contract::io::file_buffer_input{"examples/yaml/payment_config.yaml"});

PaymentConfig config{};
in >> config;
```

This is the intended usage shape:

- define a `CONTRACT` type next to the data model;
- keep required fields as normal members and optional fields as
  `std::optional<T>`;
- pass a `contract::io::window_input`-like backend to
  `contract::adapters::yaml::reader`;
- stream the YAML data into the target object with `operator>>`.

The file-backed example used by the repository is:

- [`examples/yaml_file_read.cpp`](../../examples/yaml_file_read.cpp)
- [`examples/yaml/payment_config.yaml`](../../examples/yaml/payment_config.yaml)

## What It Supports

- scalars: strings, booleans, integers, floating-point values, enums, and null;
- mappings: `key: value` objects mapped onto `CONTRACT` types;
- sequences: `- item` lists mapped onto `std::vector`, `std::array`, and tuple-like values;
- nested values: any supported value may appear recursively inside another one.

For mappings, key order in the file does not matter. The adapter matches by
field name and checks duplicates by field name.

That mapping step still goes through the real CONTRACT descriptor, so
member/reference/property access is resolved through the same descriptor
surface as the other adapters.

## Guarantees

- consumes a windowed byte-oriented IO backend with `peek()` / `consume()`;
- uses the window only as a byte cursor; YAML tokenization remains
  line-oriented inside the adapter;
- does not use `read()` or `read_view()` fallback paths;
- treats duplicate keys as errors;
- treats unknown keys as errors when the adapter is configured to reject them;
- reports missing required keys after the object scan completes;
- uses `std::optional<T>` as the built-in optional-field marker.

## How It Works

- `io` provides the windowed byte cursor.
- `reader` owns tokenization, traversal, and final diagnostic formatting.
- `codec<T>` owns type-specific parsing and block handling.
- field-aware dispatch reads the descriptor metadata and access kind before it
  decides whether to use a direct field path or a hook-driven property path.
- reader and codec operations use status/result flow internally.
- `operator>>` is a convenience boundary that converts a failed status into a
  thrown `parse_error`.

## Error Model

The reader reports failures through the same adapter diagnostic model used by
the protobuf adapter.

- `reader::read(...)` returns `parse_status::ok` or `parse_status::error`.
- `reader::error()` exposes the retained `parse_error` diagnostic after a
  failed status; `error(child)` transfers nested child diagnostics upward, and
  the local diagnostic is created lazily on the first failure.
- scalar helpers return success/failure to the caller; they do not throw as the
  normal parse protocol.
- leaf code records the local YAML failure code and stage.
- parent layers add only enclosing context, such as the current field or block.
- when a child diagnostic is transferred upward, the parent may refine the line
  or surrounding block snippet only if it knows the exact relationship between
  the nested view and the outer YAML stream.
- diagnostics include YAML-specific line and snippet data when that information
  is already available from the reader state.
- an internal developer trace is appended to the message at the end.

See [`rationale/errors.md`](../rationale/errors.md) for what a diagnostic
message is made of in general. In yaml's case specifically (using
`PaymentConfig` as declared above, where `service` is required - not
`std::optional<T>`):

```cpp
// "service" is required and missing here.
contract::adapters::yaml::reader<> in(
    contract::io::window_input{"port: 8080\nenabled: true\n"});
PaymentConfig config{};
in >> config; // throws
```

```text
yaml reader: missing required key while reading field at line 2 near "true"
[created at contract/adapters/yaml.hpp:1002 in static parse_status
contract::adapters::yaml::codec<PaymentConfig>::check_missing_field(...)
[..., Field = contract::field<PaymentConfig, 1, contract::field_kind::member,
contract::attributes<>, void, &PaymentConfig::service>]]
```

Note the missing field's own name (`service`) only shows up here, buried in
the instantiated template arguments (`Field = ...&PaymentConfig::service`) -
unlike protobuf's and compact's examples, this check does not attach the
field to the error object, so the human-readable part of the message does
not say which key is missing.

## Public Surface

- [`contract::adapters::yaml::reader`](../../include/contract/adapters/yaml.hpp)
- [`contract::adapters::yaml::options`](../../include/contract/adapters/yaml.hpp)
- [`contract::adapters::yaml::parse_status`](../../include/contract/adapters/yaml.hpp)
- [`contract::adapters::yaml::parse_error`](../../include/contract/adapters/yaml.hpp)
- [`contract::adapters::yaml::all.hpp`](../../include/contract/adapters/yaml/all.hpp)
