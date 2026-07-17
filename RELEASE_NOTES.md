# Release Notes

This file tracks user-visible changes for CONTRACT releases.

## v0.3.0

### Breaking Changes

- The direct `yaml::parse_error` convenience constructor now takes
  `(line, snippet, location)` instead of `(location, line, snippet)`. There are
  no in-tree call sites; this only affects external code that constructs the
  error type directly.

### Added

- CONTRACT can now be installed with `cmake --install` and consumed from an
  independent CMake project with
  `find_package(contract 0.3 CONFIG REQUIRED)` and the existing
  `contract::contract` target.
- The install tree includes the public headers, CMake package and version
  files, exported targets, and the Apache-2.0 license.
- A package integration test now installs CONTRACT into an isolated prefix,
  configures an external consumer through `find_package`, builds it, and runs
  it.

### CI

- GitHub Actions now uses a pinned Ubuntu 24.04 runner and tests both Debug and
  Release builds with GCC 13 and Clang 19.
- CI jobs now have read-only repository permissions, a timeout, and concurrency
  cancellation for obsolete runs from the same ref.
- The default developer preset now uses Clang 19, matching the supported Clang
  version in CI.

### Docs

- Added installation and `find_package` instructions to the root README.
- Documented narrow includes as the default integration path; family
  `all.hpp` headers remain conveniences for tests and examples.

## v0.2.0

### Breaking Changes

- The macro-generated nested type holding per-field descriptors is renamed
  from `field` to `contract_fields`. This also fixes a real collision: a
  member literally named `field` used to clash with the generated type of
  the same name.
- Per-field access hooks are no longer keyed by `contract::tag<...>`. Replace:
  - `contract_get(contract::tag<T::field::X>{}, obj)` with
    `contract_get(const T::contract_fields::X&, obj)`
  - `obj.contract_get(contract::tag<T::field::X>{})` with
    `obj.contract_get(const T::contract_fields::X&)`
  - same pattern for `contract_set`.
  There is no compatibility shim for the old syntax - this is an intentional,
  source-breaking core simplification. See [`docs/core.md`](docs/core.md)
  and [`docs/rationale/extensions.md`](docs/rationale/extensions.md) for the
  current model.
- Reference-type data members (`T& member;`) must now be declared explicitly
  with `REFERENCE(name, id)` instead of the plain `(name, id)` form. The
  previous implicit detection (attempting `&Owner::member`, which is
  ill-formed for a reference member, and silently falling back) is gone;
  C++ has no member pointer for a reference member, so this can no longer be
  inferred.
- The four boolean field-kind flags (`is_member_field`, `is_reference_field`,
  `is_property_field`) are replaced by a single `Field::kind` value of the
  new `contract::field_kind` enum (`member` / `reference` / `property`).
  `is_base_import` is unchanged.

### Added

- `contract::dispatch_field_by_id<T>(id, fn) -> bool` and
  `contract::dispatch_field_by_name<T>(key, fn) -> bool` - shared core
  utilities that map a runtime wire id/key to the matching declared field
  and invoke a callback on it, or return `false` without calling it.

### Performance

- Fixed an O(N^2) field-lookup pattern shared by the protobuf, compact, and
  yaml adapters' readers: each independently restarted a linear field scan
  from index 0 on every incoming wire field/key. All three now go through
  the shared `dispatch_field_by_id`/`dispatch_field_by_name` utilities
  above (one fused comparison pass per lookup instead of a
  restart-from-zero chain). Measurably faster on wider messages against
  both real libprotobuf and CONTRACT's own compact adapter; no regressions
  found across repeated benchmark runs on the existing scenarios.

### Fixed

- Tests were compiled with `assert()` silently compiled out, because the
  primary dev build uses `-DNDEBUG` (Release) and the test targets inherited
  that flag. Test targets now build with `-UNDEBUG`. This uncovered and
  fixed real bugs in the console and protobuf adapters, and two
  `compile_fail` tests that were failing for the wrong reason.

### Docs

- Merged the exception-policy page into a new
  [`docs/rationale/errors.md`](docs/rationale/errors.md), which now also
  documents the shared diagnostic message anatomy with real,
  compiled-and-run examples per adapter.
- [`docs/reference/benchmarks.md`](docs/reference/benchmarks.md) now covers
  all four benchmark binaries (was one), with fresh measured numbers and
  guidance on how many iterations/repeats are actually needed per tool.
- Fixed a real inaccuracy in `docs/adapters/protobuf.md#performance`: it
  claimed repeated scalar fields show no unpack slowdown, but
  `vector[100]` measurably does, for the same root cause as the previously
  documented `int25` case.
- The CONTRACT-vs-libprotobuf performance result is now surfaced in the
  root `README.md` and in `docs/rationale/positioning.md`.

## v0.1.0

Initial public release.

- Apache-2.0 license;
- root landing README;
- docs index under `docs/`;
- public contribution, security, and release policy files;
- GitHub Actions CI;
- runnable hero round-trip example.
