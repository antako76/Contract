# Release Policy

`CONTRACT` uses a release-line flow with local work branches.

## Branches

- `main` is the public release branch.
- Day-to-day work happens in a local work branch that is not part of the
  normal public history.
- `main` should stay buildable and releasable.
- Release preparation may happen in a temporary release branch if needed.
- Hotfix branches may be created when needed, but they are not part of the
  normal workflow.

The intended public history is a clean sequence of release merges and tags on
`main`, not a stream of small working commits.

## Versioning

`CONTRACT` uses Semantic Versioning.

Version tags use the format:

```text
vMAJOR.MINOR.PATCH
```

Version meaning:

- `MAJOR` changes may break the public API, documented behavior, or serialized
  format compatibility.
- `MINOR` changes add compatible functionality.
- `PATCH` changes fix bugs without changing documented public behavior or
  compatibility.

Before `1.0.0`, compatibility may still evolve, but breaking changes should
still be called out clearly in release notes.

## Releases

A release is created by merging a prepared release branch into `main` and
tagging that merge commit.

Each release should include:

- a version tag;
- a GitHub Release entry;
- a short summary of user-visible changes in [`RELEASE_NOTES.md`](RELEASE_NOTES.md)
  and the GitHub Release entry;
- compatibility notes when public API or adapter behavior changes.

## Public Surface

The public repository may include:

- [`include/contract/`](include/contract/)
- [`examples/`](examples/)
- [`tests/`](tests/)
- [`benchmarks/`](benchmarks/)
- [`docs/`](docs/)
- root project files such as [`README.md`](README.md), [`LICENSE`](LICENSE),
  [`CONTRIBUTING.md`](CONTRIBUTING.md), [`SECURITY.md`](SECURITY.md), and this
  release policy.

Code, experiments, private adapters, internal notes, credentials,
customer-specific materials, or unreleased commercial work should not be
pushed to the public repository.

If something must remain private, keep it in a private repository or out of
Git history entirely.

## Compatibility

Public API and adapter behavior should not change silently.

Changes that affect contract definitions, field identifiers, serialization
behavior, or adapter semantics must be documented in the release notes.

Security fixes may break compatibility when necessary.
