# CONTRACT Docs

This is the documentation index for the CONTRACT layer.
Use it when you want the layer map, the shared adapter contract, and the
technical notes that sit behind the repository landing page.

If you want the product-style overview and the hero example, start at the
repository [`README.md`](../README.md).

## Layer Map

- [`core.md`](core.md) - the `CONTRACT` definition model, field descriptors, and field graph.
- [`io.md`](io.md) - byte backends and convenience facades.
- [`adapters/README.md`](adapters/README.md) - the shared public adapter contract, traversal, rendering, and
  encoding; the family pages (console, binary, compact, protobuf, JSON, YAML)
  describe what differs per format.
- [`logging.md`](logging.md) - structured newline-delimited JSON logging.
- [`attributes/README.md`](attributes/README.md) - the canonical attribute model and adapter enforcement rules.
- [`include_map.md`](include_map.md) - the current public include tree and suggested bundles.
- [`reference/`](reference/) - canonical examples and benchmarks.
- [`rationale/`](rationale/) - design intent and tradeoffs:
  - [`positioning.md`](rationale/positioning.md) - why CONTRACT exists.
  - [`tradeoffs.md`](rationale/tradeoffs.md) - why the layer split and object-based API look the way they do.
  - [`extensions.md`](rationale/extensions.md) - public query surface and adapter extension points.
  - [`interoperability.md`](rationale/interoperability.md) - bridge, compatibility, and generator integration paths.
  - [`related_work.md`](rationale/related_work.md) - lessons taken from adjacent libraries without copying their structure.
  - [`compatibility.md`](rationale/compatibility.md) - versioning policy, compatibility levels, and checks.
  - [`exceptions.md`](rationale/exceptions.md) - exception policy versus status/result control flow.

## Public Header Entry Points

- [`README.md`](../README.md) at the repository root is the project landing page.
- [`include/contract/contract.hpp`](../include/contract/contract.hpp) is the canonical core entry point.
- [`include/contract/attribute.hpp`](../include/contract/attribute.hpp) is the policy entry point header.
- [`include/contract/io.hpp`](../include/contract/io.hpp) is the I/O façade header.
- [`include/contract/cout.hpp`](../include/contract/cout.hpp) is the console-first preset facade header.
- [`include/contract/logging.hpp`](../include/contract/logging.hpp) is the structured logging entry point.
