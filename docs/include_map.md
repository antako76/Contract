# CONTRACT Include Map

This page is the stable include-tree reference for the repository.
Use it when you want to know which public header to include for a given
layer, and which headers are intentionally opt-in.

## Canonical Public Entry Points

- [`include/contract/contract.hpp`](../include/contract/contract.hpp)
  - core CONTRACT model
  - DSL macros
  - field graph, traversal helpers, adapter-facing metadata
- [`include/contract/attribute.hpp`](../include/contract/attribute.hpp)
  - attribute policy entry point
  - resolution
  - validation
- [`include/contract/check.hpp`](../include/contract/check.hpp)
  - check vocabulary
- [`include/contract/security.hpp`](../include/contract/security.hpp)
  - security vocabulary
- [`include/contract/schema.hpp`](../include/contract/schema.hpp)
  - schema vocabulary
- [`include/contract/unit.hpp`](../include/contract/unit.hpp)
  - unit vocabulary
- [`include/contract/doc.hpp`](../include/contract/doc.hpp)
  - documentation vocabulary
- [`include/contract/io.hpp`](../include/contract/io.hpp)
  - I/O façade layer
- [`include/contract/cout.hpp`](../include/contract/cout.hpp)
  - console-first preset facade
- [`include/contract/logging.hpp`](../include/contract/logging.hpp)
  - structured newline-delimited JSON logging
  - event metadata, context, and named payload fields

## Adapter Entry Points

Include adapters explicitly. Do not rely on a monolithic adapter umbrella.
Each family has a lean entry-point header plus an `all.hpp` that pulls in every
supported container codec for that family.

- [`include/contract/adapters/binary.hpp`](../include/contract/adapters/binary.hpp)
  · [`binary/all.hpp`](../include/contract/adapters/binary/all.hpp)
- [`include/contract/adapters/compact.hpp`](../include/contract/adapters/compact.hpp)
  · [`compact/all.hpp`](../include/contract/adapters/compact/all.hpp)
- [`include/contract/adapters/protobuf.hpp`](../include/contract/adapters/protobuf.hpp)
  · [`protobuf/all.hpp`](../include/contract/adapters/protobuf/all.hpp)
- [`include/contract/adapters/console.hpp`](../include/contract/adapters/console.hpp)
  · [`console/all.hpp`](../include/contract/adapters/console/all.hpp)
- [`include/contract/adapters/json.hpp`](../include/contract/adapters/json.hpp)
  · [`json/all.hpp`](../include/contract/adapters/json/all.hpp)
- [`include/contract/adapters/yaml.hpp`](../include/contract/adapters/yaml.hpp)
  · [`yaml/all.hpp`](../include/contract/adapters/yaml/all.hpp)
- [`include/contract/adapters/schema.hpp`](../include/contract/adapters/schema.hpp)
  (single header)

Per-container leaf headers live under each family directory so a TU can include
only the codecs it uses:

- `binary/` and `compact/` provide `array`, `bitset`, `map`, `optional`,
  `span`, `tuple`, `unordered_map`, `variant`, `vector`.
- `protobuf/`, `console/`, and `json/` provide the same set except `span`.
- `yaml/` currently ships only `all.hpp` (no separate leaf headers).

`binary.hpp` is the lean default entry point; `binary/all.hpp` is the
full-set include for all supported standard container families. The same
entry-point/`all.hpp` split applies to every family above.

Shared adapter support headers (`adapters/base.hpp`, `adapters/base/format.hpp`,
`adapters/debug/*.hpp`) are pulled in transitively; include them directly only
when extending the adapter framework.

## Suggested Bundles

### Core-only TU

Use this when you need the CONTRACT model but not policy or adapters.

```cpp
#include <contract/contract.hpp>
```

### Policy TU

Use this when you need attribute resolution or validation.

```cpp
#include <contract/contract.hpp>
#include <contract/attribute.hpp>
```

### Vocabulary TU

Use this when you need one concrete vocabulary family.

```cpp
#include <contract/contract.hpp>
#include <contract/check.hpp>
#include <contract/security.hpp>
```

### Adapter TU

Use this when you need a concrete adapter.

```cpp
#include <contract/contract.hpp>
#include <contract/attribute.hpp>   // only if the adapter uses policy helpers
#include <contract/adapters/binary/all.hpp>
```

## Compatibility Notes

- `contract.hpp` is the canonical public core entry point.
- `core.hpp` no longer exists.
- `attribute.hpp` is the canonical policy entry point.
- `attribute_resolution.hpp` and `attribute_validation.hpp` no longer exist.
- Vocabulary wrappers are intentionally separate root headers.

## Rule of Thumb

If a TU does not need a layer, do not include it.
The public headers are split so compilation cost follows responsibility.
