# benchmarks

Opt-in benchmarks for comparing CONTRACT-based adapters against handwritten
code and selected neighboring libraries.

Keep benchmarks out of the default build until the core API and at least one
adapter are stable enough to measure.

## Binary Adapter Benchmark

Build and run:

```bash
cmake -S . -B build-bench \
  -DCONTRACT_BUILD_TESTS=OFF \
  -DCONTRACT_BUILD_EXAMPLES=OFF \
  -DCONTRACT_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench
./build-bench/benchmarks/contract_binary_adapter_benchmark
```

The current binary adapter benchmark compares CONTRACT traversal with
handwritten code across several access paths and value shapes. It reports a
ratio matrix and, when requested, quantiles for each cell. Treat it as an
opt-in regression and sanity check suite, not a stable performance promise.

You can also isolate a single row by name or by 1-based index after the rows
are sorted:

```bash
./build-bench/benchmarks/contract_binary_adapter_benchmark --row "Numeric[4 fields]"
./build-bench/benchmarks/contract_binary_adapter_benchmark --row-index 1
```

## Binary vs Protobuf Simple Benchmark

Build and run:

```bash
cmake -S . -B build-bench \
  -DCONTRACT_BUILD_TESTS=OFF \
  -DCONTRACT_BUILD_EXAMPLES=OFF \
  -DCONTRACT_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench
./build-bench/benchmarks/contract_protobuf_binary_simple_benchmark
```

This benchmark compares binary, protobuf, and compact pack/unpack on a narrow
set of simple shapes: numeric fields, owned string fields, and a nested
message made from those fields. It is intentionally small so the comparison is
easier to read than the full binary benchmark.

## Protobuf vs Compact Benchmark

Build and run:

```bash
cmake -S . -B build-bench \
  -DCONTRACT_BUILD_TESTS=OFF \
  -DCONTRACT_BUILD_EXAMPLES=OFF \
  -DCONTRACT_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench
./build-bench/benchmarks/contract_protobuf_compact_benchmark
```

This benchmark compares protobuf and compact pack/unpack with the same row
selection and reporting flow as the binary adapter benchmark, but focused on
protobuf-vs-compact scenarios: scalars, nested messages, arrays, bitsets,
tuples, variants, optional values, maps, unordered maps, and raw `u32`
ranges. It also supports `--list-rows`, `--row`, and `--row-index` so you can
isolate a single scenario.
