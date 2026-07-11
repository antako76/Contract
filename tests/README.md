# tests

Focused tests for the standalone CONTRACT layer.

## Layout

- [`unit/core/`](unit/core/) - contract field descriptors, base flattening, property fields, access hooks
- [`unit/debug/`](unit/debug/) - type names, format helpers, field comments
- [`unit/adapters/binary/`](unit/adapters/binary/) - binary adapter coverage
- [`unit/adapters/console/`](unit/adapters/console/) - console adapter golden tests
- [`compile_fail/core/`](compile_fail/core/) - current core definition rejection cases
- [`compile_fail/binary/`](compile_fail/binary/) - current binary adapter rejection cases

## Matrix

The current matrix is:

| Area | Covered now | Add first |
| --- | --- | --- |
| Core model | physical fields, property fields, access hooks, base flattening, nested base composition | more edge cases around deeper descriptor graphs and mixed base/property arrangements |
| Debug helpers | type names, field comments, string/bytes formatting helpers | attribute-driven comment metadata for new leaf kinds |
| Binary adapter | round-trip coverage, mixed composite round-trips, and compile-fail guards | wider compatibility matrix for adapter misuse and future format-specific rules |
| Console adapter | nested objects, vectors, tuples, maps, unordered maps, variants, bitsets, byte previews, truncation/alignment/max-depth/max-items edge cases, mixed integration goldens | attribute-driven sensitive-field handling, explicit native-order behavior for `std::unordered_map`, deeper nested composite cases |
| Compile-fail core | invalid ids, non-base inheritance, duplicate ids | extend only when new descriptor shapes or validation rules are added |
| Compile-fail binary | invalid read forms and forbidden pointer-like inputs | extend only when new reader/writer paths appear |

## What To Add First

1. Attribute-driven sensitive-field tests.
   - Highest value gap: the console adapter already covers shape and formatting, but not sensitive-field masking through attributes.
2. Map ordering policy.
   - Lock down the intended native-order behavior for `std::unordered_map` so the output stays intentional rather than accidentally sorted.
3. Compile-fail additions tied to new public API surface.
   - Add them only when a new misuse pattern appears.

## Fixtures

- [`contract_test_types.hpp`](contract_test_types.hpp) - shared test fixtures used by multiple unit tests.
