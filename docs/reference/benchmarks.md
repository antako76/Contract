# CONTRACT Benchmark Reference

This page defines how benchmark fixtures for CONTRACT should be structured,
for benchmarks that compare a CONTRACT adapter against equivalent handwritten
C++ code (the snapshot below is the binary adapter track).

For a benchmark against a real third-party library instead of handwritten
code, see the protobuf adapter's comparison against real libprotobuf in
[`adapters/protobuf.md#performance`](../adapters/protobuf.md#performance).

## Reference Result Snapshot

This is a local reference run from 2026-06-02. The primary number is the ratio

```text
ratio = contract cycles/op / manual cycles/op
```

The current snapshot keeps only the median ratio (`p50`) for the two most
important object-level paths:

```text
measurement                                out << obj   in >> obj
---------------------------------------------------------------
Numeric[4 fields]                          0.99         1.01
RequestEvent[3x u64]                       1.00         1.00
RoutedEvent[4x u64]                        0.98         1.08
char[8]                                    0.98         1.00
char[1000]                                 1.00         1.00
const char*[len=8]                         1.00           -
const char*[len=1000]                      0.98           -
std::array<u32>[4]                         1.01         1.00
std::array<u32>[1000]                      1.00         1.00
std::bitset[10]                            0.80         0.77
std::bitset[100]                           1.00         1.00
std::map<u32,string>[2]                    0.99         0.99
std::map<u32,string>[64]                   0.99         1.01
std::optional<u32>[1]                      0.99         1.08
std::string[len=8]                         1.15         0.97
std::string[len=1000]                      0.99         0.97
std::string_view[len=8]                    1.06         0.99
std::string_view[len=1000]                 0.99         0.99
std::tuple<u32,string,array<u32,3>>[1]     1.05         0.32
std::unordered_map<u32,string>[2]          1.09         1.02
std::unordered_map<u32,string>[64]         1.13         1.01
std::variant<u32,string,array<u32,3>>[1]   1.00         0.97
std::vector<u32>[4]                        1.34         0.63
std::vector<u32>[1000]                     0.41         0.06
```

The main thing to read is the median ratio (`p50`), with `p25` and `p75`
showing the spread. Values below `1.0` mean the CONTRACT path is faster than
the handwritten baseline for that row and column. Values above `1.0` mean the
handwritten path is faster.

## Core Principle

Each fixture must exercise one access path cleanly. A fixture must not mix
competing descriptor behavior such as physical storage access and custom hooks
unless the benchmark row is explicitly about that behavior.

The result matrix should make regressions attributable:

```text
value type -> access path -> ratio against equivalent manual code
```

## Access Paths

Benchmarks should cover these get/write paths where they apply:

```text
out << obj
field.get
member.contract_get
field.ref
```

Benchmarks should cover these read/set paths where they apply:

```text
in >> obj
field.set
member.contract_set
direct assign
```

The full object paths are intentionally grouped with the descriptor operation
they use internally:

```text
out << obj -> writer.field -> field.get
in >> obj  -> reader.field -> field.ref or field.set
```

## Fixture Rules

Use separate fixture types for separate access paths.

Plain fixtures:

```text
physical field storage only
no contract_get
no contract_set
used for field.get, field.ref, field.set direct-storage paths
```

Hooked fixtures:

```text
same value shape as the plain fixture
only the hook required by the measured row
used for member.contract_get or member.contract_set rows
```

A fixture must not be both a plain baseline and a hook target. Adding
`contract_get` or `contract_set` changes the descriptor semantics of the type
and can invalidate unrelated rows.

## Value-Type Isolation

A fixture should contain only the value type being measured unless the row is
explicitly about traversal shape.

Examples:

```text
std::string fixture        -> one std::string field
std::string_view fixture   -> one std::string_view field
std::vector<T> fixture     -> one std::vector<T> field
std::variant<T...> fixture  -> one active alternative; index + payload
inheritance fixture        -> base fields + derived field, because traversal is the subject
```

Do not add helper fields to make buffers convenient, add noise, or combine
multiple scenarios.

## Manual Baseline

Each ratio compares contract code against manual code that performs the same
semantic work.

Rules:

```text
same wire shape
same ownership behavior
same reserve policy for containers
same duplicate-key policy for maps
same iteration policy for unordered containers
same zero-copy behavior for borrowed types
```

If the contract path uses `reserve(count)`, the manual path should do the same
where the container supports it. If `std::unordered_map` writes raw iteration
order, the manual path should also use raw iteration order.

For `std::string_view`, manual read must create a view into the input bytes. It
must not copy into an owning string, because that would compare borrowed and
owning behavior.

## Wire Shape

Minimal fixtures do not mean simplified formats. Manual operations must use the
same encoded representation as the codec under test.

Examples:

```text
std::string      -> [size][bytes]
std::string_view -> [size][bytes], borrowed on read
std::vector<T>   -> [count][elements...]
std::variant<T...> -> [index][payload], active alternative only
std::map<K,V>    -> [count][key/value pairs...]
std::optional<T> -> [has_value][value?]
```

## Reporting

The primary benchmark output is a ratio matrix:

```text
ratio = contract cycles/op / manual cycles/op
```

Rows are value types. Columns are access paths. Diagnostic tables may be added
temporarily while investigating a regression, but they should not become part
of the stable benchmark output unless they cover a stable access path with a
clear manual baseline.

### Quantile Display

When quantile output is enabled, each cell shows:

```text
p25 / p50 / p75
```

The reported ratio remains the median (`p50`). The quartiles are displayed to
make noise and skew visible without changing the primary metric.

Coloring follows the same priority order:

```text
red    -> p25 > 1.2
yellow -> p25 <= 1.2 and p50 > 1.2
```

Red has priority over yellow. Yellow is applied only to `p50`; `p25` and
`p75` remain uncolored. This keeps the table readable while still flagging
stable regressions first.

The row label follows the same priority:

```text
red    -> at least one red cell in the row
yellow -> no red cells, but at least one yellow cell
none   -> no highlighted cells
```

Legend:

```text
p25 -> lower quartile
p50 -> median, primary ratio
p75 -> upper quartile
red -> stable regression, full cell
yellow -> median regression, p50 only
```

## Adding A New Value Type

When adding a new benchmarked value type:

```text
1. Add a minimal plain fixture.
2. Add separate hooked fixtures for contract_get and contract_set if needed.
3. Add manual operations for each measured access path.
4. Verify that manual and contract paths use the same wire and ownership model.
5. Add one row to the ratio matrix.
```

The benchmark should stay boring: clean fixture, clean path, clean manual
baseline.
