# CONTRACT

`CONTRACT` is a compile-time contract layer for native C++ structs.
Put one schema next to the type, then reuse it for readable debug output,
structured logging, YAML configuration, and binary/protobuf/compact transport
to files or the network — all from the same declaration, without runtime
reflection or per-format field mapping.

`contract` is project by Ilya Korolev (Antako). 

This repository contains the active `CONTRACT` implementation and the
documentation that explains how the layer is split.

This project is licensed under the Apache License, Version 2.0. See
[`LICENSE`](LICENSE) for the full text.

## What You Get

- A compile-time schema that lives with the C++ type.
- Stable field ids, names, types, and access rules declared once.
- Typed traversal over physical fields, nested values, and imported bases.
- No runtime registry and no per-format handwritten field mapping.
- Object-based public APIs for human-readable and binary adapter families.
- A contract model that can grow into more adapters without being rewritten.

## Why It Matters

The same data shape usually has to survive across debug output, logs, file
dumps, network payloads, and schema checks.

Without a shared contract layer, each format grows its own mapping and its own
edge-case handling. That creates drift.

`CONTRACT` makes the schema explicit once, next to the type, and lets adapters
reuse it consistently. That keeps formats aligned and gives each adapter a
stable source of truth.

## Example - One Contract, Binary Round-Trip, Debug Output

The example below is intentionally small but complete: one contract writes a nested C++ object into a binary buffer, reads it back, and prints the restored object for inspection.

```cpp
#include <contract/contract.hpp>
#include <contract/adapters/binary/all.hpp>
#include <contract/cout.hpp>
#include <array>
#include <optional>
#include <string>
#include <vector>

struct UserProfile {
    std::string username;
    std::string team;
    unsigned int employee_id;

    CONTRACT(UserProfile,
        (username, 1),
        (team, 2),
        (employee_id, 3)
    )
};

struct SourceInfo {
    std::string source;
    std::string tenant;

    CONTRACT(SourceInfo,
        (source, 1),
        (tenant, 2)
    )
};

struct SearchDocument : public SourceInfo {
    unsigned long long document_id;
    std::string title;
    std::vector<std::string> tags;
    std::optional<UserProfile> owner;

    CONTRACT(SearchDocument,
        BASE(SourceInfo, 10),
        (document_id, 1),
        (title, 2),
        (tags, 3),
        (owner, 4)
    )
};

int main() {
    SearchDocument event{
        SourceInfo{"ingest/api", "payments"},
        42,
        "Payment retry policy",
        {"payments", "retry", "critical"},
        UserProfile{"alex", "search", 1042}
    };
    std::array<unsigned char, 1024> buffer{};
    SearchDocument restored{};

    contract::adapters::binary::writer<> out(buffer.data());
    out << event; // binary output to buffer

    contract::adapters::binary::reader<> in(buffer.data());
    in >> restored; // read the same shape back into another variable

    contract::cout.debug() << restored; // debug output after round-trip
}
```

Rendered output:

```text
SearchDocument:
  source: "ingest/api"             # #11 std::string, SourceInfo+10
  tenant: "payments"               # #12 std::string, SourceInfo+10
  document_id: 42                  # #1 u64
  title: "Payment retry policy"    # #2 std::string
  tags:                            # #3 std::vector<std::string>, size=3
    - "payments"                   # [0]
    - "retry"                      # [1]
    - "critical"                   # [2]
  owner:                           # #4 std::optional<UserProfile>
    UserProfile:
      username: "alex"             # #1 std::string
      team: "search"               # #2 std::string
      employee_id: 1042            # #3 u32
```

Why this example matters:

- one schema covers ids, types, container sizes, provenance, and nested ownership;
- the contract lives next to the type and is declared once at compile time;
- base fields, nested values, containers, and optionals share one traversal
  model;
- the same object-based entry point feeds human-readable inspection, binary
  transport, and round-tripping into another variable;
- the whole shape is visible at a glance without hunting through adapter code;
- the output reads like a structured document, not a formatter dump.

## Performance

The compile-time schema isn't just convenient - it's fast. Measured against
real libprotobuf (3.21.12), with wire output verified byte-for-byte
identical: CONTRACT's protobuf adapter is faster in nearly every scenario,
typically taking 0.3x-0.8x of libprotobuf's time to pack and 0.4x-0.9x of its
time to unpack. See
[`docs/adapters/protobuf.md#performance`](docs/adapters/protobuf.md#performance)
for the full breakdown, known exceptions, and how to run the benchmark
yourself.

## Where To Go Next

- [`docs/README.md`](docs/README.md) for the layer docs index
- [`include/contract/`](include/contract/) for the public headers
- [`examples/`](examples/) for public usage examples
- [`tests/`](tests/) for coverage and compatibility checks
- [`docs/reference/examples.md`](docs/reference/examples.md) for additional usage examples
- [`RELEASE_POLICY.md`](RELEASE_POLICY.md) for branching, versioning, and release rules
- [`RELEASE_NOTES.md`](RELEASE_NOTES.md) for user-visible release notes

## CMake Integration

When CONTRACT is part of the same source tree, add it directly and link the
interface target:

```cmake
add_subdirectory(path/to/contract)
target_link_libraries(your_target PRIVATE contract::contract)
```

To install CONTRACT and consume it as a package:

```sh
cmake -S . -B build -DCONTRACT_BUILD_EXAMPLES=OFF -DCONTRACT_BUILD_TESTS=OFF
cmake --install build --prefix /your/install/prefix
```

```cmake
find_package(contract 0.3 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE contract::contract)
```

Include the narrowest header that provides the API you use. Family `all.hpp`
headers are conveniences for examples and tests; they are not the default
integration path because they increase the amount of code each translation
unit must parse. See [`docs/include_map.md`](docs/include_map.md) for the public
include tree.

## Key Headers

- [`include/contract/contract.hpp`](include/contract/contract.hpp) for the core contract model and DSL macros.
- [`include/contract/attribute.hpp`](include/contract/attribute.hpp) for the policy entry point.
- [`include/contract/cout.hpp`](include/contract/cout.hpp) for the console-first preset facade.
- [`include/contract/io.hpp`](include/contract/io.hpp) for the I/O façade layer.
- [`include/contract/adapters/console.hpp`](include/contract/adapters/console.hpp) for the console adapter.
- [`include/contract/adapters/binary.hpp`](include/contract/adapters/binary.hpp) for the lean binary adapter entry point.
- [`include/contract/adapters/binary/all.hpp`](include/contract/adapters/binary/all.hpp) for the full binary adapter family set.
- [`docs/include_map.md`](docs/include_map.md) for the current include tree.

## Project Layout

- [`include/contract/`](include/contract/) contains the public header-only CONTRACT library.
- [`include/contract/adapters/`](include/contract/adapters/) contains optional public adapters over the core
  traversal API.
- [`include/contract/detail/`](include/contract/detail/) contains private implementation helpers.
- [`docs/README.md`](docs/README.md) is the documentation entry point for the layer.
- [`examples/`](examples/) contains small public API examples.
- [`src/`](src/) is reserved for future non-template implementation units.
- [`tests/`](tests/) contains focused correctness and compatibility tests.
