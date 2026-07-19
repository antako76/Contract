# CONTRACT Benchmark Reference

This page defines how benchmark fixtures for CONTRACT should be structured,
and gives a small, dated sample of what each benchmark currently reports.
Sample numbers rot fast - treat every number below as illustrative, not a
promise, and re-run the binary yourself for anything you actually need to
rely on.

## Benchmark Suite

Four benchmark binaries live under `benchmarks/`. The benchmark suite is
opt-in through `-DCONTRACT_BUILD_BENCHMARKS=ON` except where noted:

```text
contract_binary_adapter_benchmark
  binary_adapter_benchmark.cpp
  CONTRACT's binary adapter vs. equivalent handwritten manual C++ code.

contract_protobuf_compact_benchmark
  protobuf_compact_adapter_benchmark.cpp
  CONTRACT's protobuf adapter vs. CONTRACT's compact adapter, head to head
  (both CONTRACT-native, no manual baseline).

contract_protobuf_binary_simple_benchmark
  protobuf_binary_simple_benchmark.cpp
  All three CONTRACT-native adapters together: binary as the baseline,
  protobuf and compact reported as ratios against it.

contract_protobuf_reference_benchmark
  protobuf_reference_benchmark.cpp
  CONTRACT's protobuf adapter vs. real libprotobuf. Gated behind
  -DCONTRACT_BENCH_WITH_PROTOBUF=ON (off by default, so the core library and
  tests carry no protobuf dependency). See its own dedicated writeup at
  [`adapters/protobuf.md#performance`](../adapters/protobuf.md#performance)
  instead of duplicating results here.
```

Run any of them with `--iterations N` (larger N = less noise; `--quantiles`
and `--color` are supported by the binary adapter benchmark, see
[Quantile Display](#quantile-display) below):

```sh
cmake --preset default -DCONTRACT_BUILD_BENCHMARKS=ON
cmake --build build-clang19 --target contract_binary_adapter_benchmark
./build-clang19/benchmarks/contract_binary_adapter_benchmark --iterations 50000
```

## How Many Iterations You Actually Need

`--iterations N` controls how many operations one *repeat* measures; each
benchmark also has a fixed, compiled-in repeat count that controls how many
independent repeats it takes internally before reporting quantiles:

```text
contract_binary_adapter_benchmark            repeats = 101 (compiled in)
contract_protobuf_compact_benchmark          repeats = 7   (compiled in)
contract_protobuf_binary_simple_benchmark    repeats = 7   (compiled in)
contract_protobuf_reference_benchmark        repeats = 7   (compiled in)
```

This matters in practice: with 101 internal repeats, the binary adapter
benchmark's own quantiles (`p25`/`p50`) are usually stable enough from a
single invocation at `--iterations 50000`-`500000`. The other three only take
7 internal repeats, which is not enough by itself - a single invocation, even
at `--iterations 200000`, can still swing 10-30% run to run on a scenario
with a real but modest effect size (this was measured directly while
developing this benchmark suite: a real fix looked like a 20%+ regression on
one run and a clear improvement on the next).

For those three, when the question is "did this change actually help or
hurt" rather than "roughly how big is this number", run the binary multiple
times (5-12 independent invocations) and compare medians, not single runs.
As a check for whether an apparent difference is real: also re-run the
*same* binary against itself a few times first - if its own run-to-run spread
is close to or larger than the difference you're trying to measure, that
difference is noise, not signal.

## Reference Result Snapshot

A local reference run from 2026-07-12, a handful of rows per benchmark, not
the full table (run the binaries yourself for the rest). Column meaning is
explained in [Access Paths](#access-paths) below. Per the note above, the
binary adapter numbers (101 internal repeats) are a single invocation; the
compact-vs-protobuf numbers (7 internal repeats) are a single run shown for
illustration only, not a verified median - treat any close-to-1.0 ratio there
as noise until you've checked it across several runs yourself. The
protobuf-vs-libprotobuf numbers further below are a median of 5 runs, since
that benchmark's effect sizes matter enough to be worth verifying properly.

Binary adapter (`contract_binary_adapter_benchmark --iterations 1000`,
ratio = contract/manual, `>1` means manual was faster; `-` = path not
applicable for that type):

```text
measurement                                 out   get   ref  in(view)  in(copy)   set
Numeric[4 fields]                          1.01  1.00  0.98      1.00      1.00  1.00
std::array<u32>[4]                         0.95  1.00  1.00      1.00      1.00  0.98
std::bitset[10]                            1.00  1.00  1.00      1.00      0.93  0.99
std::optional<u32>[1]                      1.00  1.00  1.00      1.00      1.00  0.99
std::string_view[len=8]                    1.04  1.00  1.00      1.00         -  0.96
std::string[len=8]                         1.17  1.03  1.00      1.11      1.12  1.06
const char*[len=8]                         0.94  1.00  1.00         -         -  0.97
std::vector<u32>[1000]                     1.00  0.98  0.99      1.00      1.01  0.97
std::map<u32,string>[2]                    1.01  0.98  0.99      1.06      1.04  1.00
std::unordered_map<u32,string>[2]          1.21  0.99  0.99      1.02      1.12  1.00
std::tuple<u32,string,array<u32,3>>[1]     1.10  0.98  1.01      0.32      0.33  0.46
std::variant<u32,string,array<u32,3>>[1]   1.16  1.00  1.00      0.92      1.42  0.99
```

**The takeaway**: across almost every value type and access path, CONTRACT's
binary adapter costs the same as the handwritten manual code performing the
same work (ratio within ~0.05 of 1.0) - the compile-time descriptor layer is
not adding runtime overhead over hand-rolled field access, not just "pretty
close". `field.get`/`field.ref` in particular sit at 1.00 almost everywhere,
since those compile down to the same direct member access either way. The
two visible exceptions (`tuple`'s `in(view)`/`in(copy)` at 0.32-0.33 -
CONTRACT faster - and `variant`'s `in(copy)` at 1.42 - manual faster) come
from how each side's manual baseline is written for that specific container
shape, not from CONTRACT overhead; run `--row "std::tuple..."` yourself with
`--quantiles` to see the breakdown.

Compact vs. protobuf (`contract_protobuf_compact_benchmark --iterations
100000`, size in bytes, pack/unpack in ns/op, ratio = compact/protobuf):

```text
scenario         proto sz  compact sz  proto pack  compact pk  c/p   proto un  compact un  c/p
array[4]              6         7           9.06        6.58   0.73     14.00      7.30   0.52
bitset[10]             4         5          11.01        2.64   0.24     10.75      5.84   0.54
deep_nested           75        81         117.40      124.46   1.06    116.13     77.44   0.67
i64                    7         6          10.76       15.18   1.41     13.46     13.62   1.01
map[2]                30        26          25.46       22.67   0.89     99.08     79.76   0.80
```

`deep_nested` (4 levels, 5-7 fields each) is closer to a real DTO shape than
the single-field toy scenarios above and below it - see it and its neighbors
for the more representative numbers.

CONTRACT vs. real libprotobuf (`contract_protobuf_reference_benchmark
--iterations 500000`, built with `-DCONTRACT_BENCH_WITH_PROTOBUF=ON`), measured
with Clang 19 (the `default` CMake preset's compiler - see
`CMakePresets.json`). GCC reproduces the same byte-for-byte wire parity but
shows a larger unpack gap specifically on the `int25` row below; re-run the
binary under your own toolchain rather than assuming these exact ratios carry
over to a different compiler. The tool's own header:

```text
size in bytes; pack/unpack in ns/op (median of 7); x column = contract/protobuf ratio (>1 means contract is slower)
```

Each single invocation already reports a median of its own 7 internal
repeats - the numbers below take the median of 5 *independent invocations*
of that (so a median of medians, ~90s total), per
[How Many Iterations You Actually Need](#how-many-iterations-you-actually-need).

This snapshot replaces an earlier one taken before a reset-methodology bug in
the benchmark itself was fixed: the unpack loop reset its persistent `target`
via `target = Value{}` between operations, which discards whatever buffer
capacity a `std::string` field had already accumulated and forces a fresh
allocation on every single iteration - unlike libprotobuf's own `Clear()`,
which resets in place and keeps existing capacity. String-bearing scenarios
(`text`, `wide[10 fields]`, `all_strings[6]`, `str25[25 fields]`, `nested`)
now reset the same way `Clear()` does, so their unpack numbers below reflect
actual decode cost rather than benchmark-induced allocator churn - `str25`
in particular moved from a x0.58 to a x0.34 unpack ratio purely from this
fix. Pack numbers were never affected by this bug (the pack loop has no
persistent target to reset); where pack ratios below differ from the
previous snapshot, that is ordinary run-to-run/machine variance, the same
kind [How Many Iterations You Actually Need](#how-many-iterations-you-actually-need)
already warns about - re-run the binary yourself rather than trusting either
snapshot as a permanent number.

This is the full scenario set (only 14 rows, so no reason to trim it the way
the two larger benchmarks above are trimmed):

```text
scenario           size      pack(c/p)     x      unpack(c/p)     x
numeric              25   16.6 /  20.5  0.81     31.1 /  30.3  1.03
text                 18    7.9 /  22.3  0.35     14.9 /  29.0  0.51
nested               47   28.3 /  41.4  0.68     50.2 /  88.4  0.57
vector[4]             6   11.9 /  22.3  0.53     15.2 /  20.1  0.76
vector[25]           27   49.5 /  50.4  0.98     36.3 /  48.6  0.75
vector[100]         171  228.6 / 180.3  1.27    153.7 / 135.9  1.13
wide[10 fields]     110   43.0 /  55.6  0.77     56.2 /  97.1  0.58
string_vector[4]     43   24.4 /  47.4  0.52     33.4 /  70.7  0.47
string_vector[50]   465  210.9 / 380.0  0.55    372.4 / 673.0  0.55
all_strings[6]       94   36.0 /  66.1  0.55     42.9 / 119.5  0.36
all_numbers[8]       42   30.1 /  29.4  1.02     34.0 /  43.6  0.78
bytes[32]            17   21.5 /  17.9  1.20     20.1 /  25.2  0.80
int25[25 fields]     78   53.2 /  49.5  1.08     84.6 /  71.9  1.18
str25[25 fields]    272  122.7 / 194.9  0.63    153.1 / 452.6  0.34
```

A single ratio number from one aggregated table isn't enough to call a
scenario "faster" or "slower" - the same 5 independent invocations that
produced the medians above were checked row-by-row for whether the ratio
stayed on the same side of 1.0 in *every* one of the 5, not just in the
pooled median. That gives three honest buckets instead of one noisy
threshold at 1.0:

- **CONTRACT confirmed faster** (ratio `< 1` in all 5 runs): 9 of 14 pack
  rows, 11 of 14 unpack rows.
- **protobuf confirmed faster** (ratio `> 1` in all 5 runs): pack -
  `vector[100]`, `bytes[32]`, `int25`; unpack - `vector[100]`, `int25`.
  `bytes[32]`'s pack ratio (x1.20) looks close to parity in the table above.
  it isn't - all 5 runs measured it above 1.0 (range 1.12-1.24), a small but
  consistent and repeatable slowdown, not noise.
- **Genuinely undecided** (the ratio crosses 1.0 across the 5 runs, sign
  flips run to run): pack - `vector[25]`, `all_numbers[8]`; unpack -
  `numeric`. These are the only three rows where the table's ratio being
  close to 1.0 actually means "can't tell" rather than "small real effect."

`int25` and `vector[100]` are the two scenarios with both a confirmed pack
*and* unpack slowdown - both many-cheap-element shapes with no strings to
mask dispatch/decode cost, and neither has a `std::string` field for the
reset fix above to affect. See
[`adapters/protobuf.md#performance`](../adapters/protobuf.md#performance)
for the full explanation and what was tried against `int25` specifically.

Compiler choice measurably changes the `int25` row above: GCC's inliner
fully unrolls `read_field` into `read_message` for every field instead of
keeping it as a compact per-field call the way Clang does, which has been
observed to push `int25`'s unpack ratio well above the x1.18 figure shown
here. This is a backend code-generation difference, not a difference in
what the library asks either compiler to do - no compiler-specific code
path exists in the adapter. A canonical GCC snapshot (same 5-invocation
median methodology as above) has not been added yet; treat any GCC number
you measure yourself as its own reference point rather than assuming parity
with the Clang table above.

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

The binary adapter benchmark's current columns follow this directly: `out`,
`get`, `ref` measure the write side; `in(view)` and `in(copy)` are two
variants of the full `in >> obj` read (zero-copy borrowed vs. owning), and
`set` measures `field.set` directly.

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

This section applies to the two benchmarks that compare against handwritten
code (binary adapter, and the binary/protobuf/compact three-way comparison).
The compact-vs-protobuf benchmark compares two CONTRACT adapters directly and
has no manual baseline.

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

When quantile output is enabled (`--quantiles`), each cell shows:

```text
cost target[p50] / cost base[p50] ~ ratio[p50] (ratio p25)
```

For example: `8.6 / 7.8 ~ 1.03 (0.91)` - contract's median cost, manual's
median cost, the median ratio, then the p25 ratio in parentheses. The ratio
is the median of the per-repeat ratios, not `target[p50] / base[p50]` (those
can disagree slightly - the ratio is computed per repeat, then its own
quantiles are taken separately from each side's cost quantiles). The reported
primary metric remains the median ratio (`p50`); the `p25` figure is shown to
make noise and skew visible without changing the primary metric.

Coloring follows the same priority order:

```text
red    -> p25 > 1.2
yellow -> p25 <= 1.2 and p50 > 1.2
```

Red has priority over yellow. Yellow is applied only to `p50`; `p25` remains
uncolored. This keeps the table readable while still flagging stable
regressions first.

The row label follows the same priority:

```text
red    -> at least one red cell in the row
yellow -> no red cells, but at least one yellow cell
none   -> no highlighted cells
```

Legend:

```text
p25 -> lower quartile (of the ratio)
p50 -> median, primary ratio
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
