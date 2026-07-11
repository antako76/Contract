# CONTRACT Docs

This is the documentation index for the CONTRACT layer.
Use it when you want the layer map, the shared adapter contract, and the
technical notes that sit behind the repository landing page.

If you want the product-style overview and the hero example, start at the
repository [`README.md`](../../README.md).

## How To Read This Tree

- Start with [`adapters/README.md`](adapters/README.md) if you want the shared public adapter contract.
- Read [`core.md`](core.md) and [`io.md`](io.md) when you want the model split and the byte split.
- Use [`attributes/README.md`](attributes/README.md) when you want the attribute model and adapter handling rules.
- Use [`include_map.md`](include_map.md) when you want the current public include tree.
- Use the family pages when you want adapter-specific behavior.
- Use [`reference/`](reference/) when you want canonical examples or benchmarks.
- Use [`rationale/`](rationale/) when you want design intent, tradeoffs, or roadmap context.

## Layer Map

- [`core.md`](core.md) - the `CONTRACT` definition model and field graph.
- [`io.md`](io.md) - byte backends and convenience facades.
- [`adapters/README.md`](adapters/README.md) - traversal, rendering, encoding, and public adapter APIs.
- [`logging.md`](logging.md) - structured newline-delimited JSON logging.
- [`attributes/README.md`](attributes/README.md) - the canonical attribute model and adapter enforcement rules.
- [`include_map.md`](include_map.md) - the current public include tree and suggested bundles.
- [`rationale/positioning.md`](rationale/positioning.md) - why CONTRACT exists and the design tradeoffs.
- [`rationale/tradeoffs.md`](rationale/tradeoffs.md) - why the layer split and object-based API look the way they do.
- [`rationale/extensions.md`](rationale/extensions.md) - public query surface and adapter extension points.
- [`rationale/interoperability.md`](rationale/interoperability.md) - bridge, compatibility, and generator integration paths.
- [`rationale/related_work.md`](rationale/related_work.md) - lessons taken from adjacent libraries without copying their structure.
- [`rationale/compatibility.md`](rationale/compatibility.md) - versioning policy, compatibility levels, and checks.
- [`rationale/exceptions.md`](rationale/exceptions.md) - exception policy versus status/result control flow.
- [`rationale/roadmap.md`](rationale/roadmap.md) - MVP shape, type support targets, and adapter pack direction.

## What Lives Where

- [`core.md`](core.md) defines the class-level contract model.
- [`io.md`](io.md) defines byte backends and convenience facades.
- [`adapters/README.md`](adapters/README.md) defines the shared public adapter contract and execution split.
- [`attributes/README.md`](attributes/README.md) defines the canonical attribute model and adapter handling rules.
- [`adapters/console.md`](adapters/console.md) and [`adapters/binary.md`](adapters/binary.md) define family-specific behavior.
- [`include_map.md`](include_map.md) defines the current public include tree.
- [`reference/`](reference/) contains canonical examples and benchmarks.
- [`rationale/`](rationale/) contains positioning and design tradeoffs.

## Public Entry Points

- [`README.md`](../../README.md) at the repository root is the project landing page.
- [`include/contract/contract.hpp`](../../include/contract/contract.hpp) is the canonical core entry point.
- [`include/contract/attribute.hpp`](../../include/contract/attribute.hpp) is the policy entry point header.
- [`include/contract/io.hpp`](../../include/contract/io.hpp) is the I/O façade header.
- [`include/contract/cout.hpp`](../../include/contract/cout.hpp) is the console-first preset facade header.
- [`include/contract/logging.hpp`](../../include/contract/logging.hpp) is the structured logging entry point.
- [`attributes/README.md`](attributes/README.md) is the attribute model and adapter enforcement index.
- [`adapters/README.md`](adapters/README.md) is the shared public adapter contract.
- [`include_map.md`](include_map.md) is the include-tree reference.
- [`core.md`](core.md) is the class-level contract model.
- [`io.md`](io.md) is the byte backend and facade split.

## Shared Rules

- Keep shared contracts in the layer docs.
- Keep adapter-specific behavior in the family pages.

## Start Here

- [Adapter Layer and Public Contract](adapters/README.md)
- [Repository README](../../README.md) for the landing-page view
