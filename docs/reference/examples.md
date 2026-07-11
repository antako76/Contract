# CONTRACT Examples

This page collects short usage examples for the current CONTRACT API.

The goal is not to document every edge case. The goal is to show the canonical
shape of the public object-based API in a compact form.

## One Schema, Many Destinations

The examples below cover one adapter each. This one chains three of them for
the same type to show what "describe once" actually buys you: no per-format
mapping code, just a different adapter at each step.

```cpp
struct PaymentConfig {
    std::string service;
    std::uint32_t port = 0;
    bool enabled = false;
    std::vector<std::string> tags;

    CONTRACT(PaymentConfig, (service, 1), (port, 2), (enabled, 3), (tags, 4))
};

// Read once, from a YAML config file.
contract::adapters::yaml::reader<contract::io::file_input> yaml_in(
    contract::io::file_input{"config/payment.yaml"});
PaymentConfig config{};
yaml_in >> config;

// Same value, one structured log record.
contract::logging::logger log{output};
log.info("config_loaded", "Loaded payment config",
    contract::logging::attribute("config", config));

// Same value, compact wire transport (to a file or over the network).
std::array<unsigned char, 256> buffer{};
contract::adapters::compact::writer<> out(buffer.data(), buffer.size());
out << config;
```

`PaymentConfig` is declared once. Nothing here writes a YAML parser, a JSON
log encoder, or a wire codec by hand — each adapter reuses the same
`CONTRACT(...)` declaration.

## Console

The console-first facade is the short human-readable entry point:

```cpp
contract::cout << event;
contract::cout.debug() << event;
contract::cout.value() << event;
contract::cout.with(custom_options) << event;
```

- [`contract::cout`](../../include/contract/cout.hpp) starts from the console-first preset.
- `debug()` adds richer provenance such as imported base offsets.
- `value()` keeps the same object API but switches to the quieter value view.
- `with(custom_options)` is the universal way to override the current config.

Concrete example:

```cpp
struct UserProfile {
    std::string name = "maria";
    std::string role = "operator";

    CONTRACT(UserProfile, (name, 1), (role, 2))
};

struct RequestEvent : public UserProfile {
    unsigned long long id = 42;
    std::string route = "/api/payments/{id}";
    std::vector<std::string> tags = {"payment", "critical"};
    std::optional<UserProfile> profile = UserProfile{"maria", "operator"};

    CONTRACT(RequestEvent, BASE(UserProfile, 20), (id, 1), (route, 2), (tags, 3), (profile, 4))
};

RequestEvent event;

contract::cout << event;         // console-first schema view
contract::cout.debug() << event; // schema + provenance
contract::cout.value() << event; // quiet value view
```

The example is intentionally small but it shows the shape that matters:

- a base class imported through `BASE(Type, offset)`;
- a nested `std::optional<UserProfile>`;
- a repeated field;
- a console-first debug preset with provenance.

Rendered output shape:

```text
RequestEvent:
  name: "maria"                        # #21 std::string
  role: "operator"                     # #22 std::string
  id: 42                               # #1 u64
  route: "/api/payments/{id}"          # #2 std::string
  tags:                                # #3 std::vector<std::string>, size=2
    - "payment"                        # [0]
    - "critical"                       # [1]
  profile:                             # #4 std::optional<UserProfile>
    UserProfile:
      name: "maria"                    # #1 std::string
      role: "operator"                 # #2 std::string

# debug
RequestEvent:
  name: "maria"                        # #21 std::string, UserProfile+20
  role: "operator"                     # #22 std::string, UserProfile+20
  id: 42                               # #1 u64
  route: "/api/payments/{id}"          # #2 std::string
  tags:                                # #3 std::vector<std::string>, size=2
    - "payment"                        # [0]
    - "critical"                       # [1]
  profile:                             # #4 std::optional<UserProfile>
    UserProfile:
      name: "maria"                    # #1 std::string
      role: "operator"                 # #2 std::string
```

Value output is the same tree without comments:

```text
RequestEvent:
  name: "maria"
  role: "operator"
  id: 42
  route: "/api/payments/{id}"
  tags:
    - "payment"
    - "critical"
  profile:
    UserProfile:
      name: "maria"
      role: "operator"
```

If a neutral stream-like facade is needed, use [`contract::io::cout`](../../include/contract/io/cout.hpp).

```cpp
contract::io::cout << event;
```

## Binary

The binary adapter uses the same object-based contract:

```cpp
contract::adapters::binary::writer<> out(buffer.data());
out << event;
out.with(custom_binary_options) << event;

contract::adapters::binary::reader<> in(buffer.data());
in >> event;
in.with(custom_binary_options) >> event;
```

Binary does not need named presets such as `schema()` or `debug()`. The common
shape is still useful because it keeps configuration and object entry points
consistent across adapter families.

Concrete example:

```cpp
std::array<std::byte, 1024> buffer{};

contract::adapters::binary::writer<> out(buffer.data());
out << event;

contract::adapters::binary::reader<> in(buffer.data());
in >> event;
```

The same object-based entry points stay readable even when the wire shape is
native binary rather than human-readable text.

## YAML

The YAML adapter reads structured config data from an IO backend and reuses the
same object-based contract:

```cpp
contract::adapters::yaml::reader<contract::io::file_input> in(
    contract::io::file_input{"examples/yaml/payment_config.yaml"});

PaymentConfig config{};
in >> config;

contract::cout.debug() << config;
```

Concrete file-backed example:

- [`examples/yaml_file_read.cpp`](../../examples/yaml_file_read.cpp)
- [`examples/yaml/payment_config.yaml`](../../examples/yaml/payment_config.yaml)

## Logging

`contract::logging` builds newline-delimited JSON log records on top of the
JSON adapter. A whole `CONTRACT` object can be logged as a single attribute
value, not just scalars:

```cpp
using contract::logging::attribute;

contract::logging::logger log{output};

log.info("config_loaded",
    contract::logging::format("Loaded config for {}", config.service),
    attribute("config", config),
    attribute("port", config.port));
```

Each call writes one record, e.g.:

```json
{"timestamp":"2026-06-14T12:34:56.789Z","severity":"info","severity_number":9,"name":"config_loaded","body":"Loaded config for billing","context":{},"attributes":[{"name":"config","value":{...}},{"name":"port","value":8443}]}
```

Fields marked `contract::security::secret()`/`no_log()` are handled by the
JSON adapter's security mode (see [`logging.md`](../logging.md) and
[`adapters/json.md`](../adapters/json.md)); a runnable example with a redacted
field is in [`examples/logging.cpp`](../../examples/logging.cpp).

## Core Model

The contract metadata itself stays type-first:

```cpp
auto def = contract::contract_of<RequestEvent>();
auto fields = contract::flattened_fields_of<RequestEvent>();
auto name = contract::type_name<RequestEvent>();
```

The adapter reads from the contract model; the contract model does not depend
on adapter behavior.

## Example Rules

- Prefer tiny examples that show the current public shape.
- Prefer [`contract::cout`](../../include/contract/cout.hpp) for console examples.
- Prefer `writer << value` / `reader >> value` for adapter examples.
- Prefer `with(options)` when the example needs to show configuration.
- Do not duplicate benchmark methodology here.
