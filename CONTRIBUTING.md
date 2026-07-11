# Contributing to CONTRACT

This repository is kept intentionally small and source-first. Contributions
should preserve that shape: clear public headers, focused examples, and tests
that prove the public API still behaves as documented.

## Compatibility

`CONTRACT` is designed around stable contract definitions.

Unless explicitly documented, pull requests should not:

- change existing field identifiers;
- reuse removed field identifiers;
- change serialization semantics of existing adapters;
- break compatibility of documented examples.

Compatibility-affecting changes should be called out clearly in the PR
description.

## Public API

Prefer extending existing abstractions before introducing new public types,
macros, customization points, or configuration mechanisms.

New public API should have a clear use case that cannot be expressed through
the existing model.

## Architecture

Keep the core contract model independent from specific serialization formats.
Adapter-specific behavior belongs in adapter layers and should not leak into
the core contract definitions.

## Examples

Examples are treated as executable documentation.

Prefer updating an existing example before introducing a new one.

## Before You Open a PR

- Read the root [`README.md`](README.md) for the project landing page.
- Read [`docs/contract/README.md`](docs/contract/README.md) for the layer docs index.
- See [`RELEASE_POLICY.md`](RELEASE_POLICY.md) for branching, versioning, and release rules.
- Prefer changes that keep the public surface small and explicit.
- Keep shared contract rules in the layer docs, and adapter-specific behavior in the family pages.
- Keep new or updated examples runnable.

## What Good Changes Look Like

- Public API changes are reflected in the matching headers under
  [`include/contract/`](include/contract/).
- User-facing behavior changes are covered by tests in [`tests/`](tests/).
- Example-facing changes are shown in [`examples/`](examples/).
- Documentation changes keep the root README as the project landing page and
  [`docs/contract/README.md`](docs/contract/README.md) as the docs index.

## Validation

Run the local checks that match your change:

- configure and build the project
- run `ctest`
- run any touched example executable

If you change public headers or adapter behavior, make sure the relevant
example or test still compiles and passes.

## Style Notes

- Keep prose direct and factual.
- Use markdown links for references to docs and code.
- Keep new docs consistent with the current README split.
- Keep headers, examples, and tests under the Apache-2.0 license headers used
  elsewhere in the repository.
