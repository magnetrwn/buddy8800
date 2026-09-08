# Trace performance

Press `x` in the TUI to toggle reporting. Disabled mode calls `step<false>()`
and `interrupt<false>()` on the same CPU; their executor specialization removes
the `_trace` calls with `if constexpr`. It does not merely install an empty
observer. Operands are no longer read a second time for tracing, instruction
strings/history are not built, and registers are not sampled/formatted.

The enabled frontend executes at most 4,096 instructions per iteration and
sleeps 25 ms, an upper bound of about 164,000 instructions/s before execution
and drawing costs. With reporting disabled it instead executes continuously in
33 ms time slices, checking time every 256 instructions. It does not sleep or
cap instructions per frame; the remaining overhead is periodic machine-panel
updates and control polling. Pause and single-step still work.

## Measurements

Measured in this Linux jail using GCC 16.1.1, `-Ofast -flto`, with no concurrent
builds. These are medians of five runs of a synthetic RAM/8080 instruction loop
on the card bus, including the normal per-instruction IRQ check. They exclude
terminal drawing and serial syscalls; real program throughput will differ.

| Interpreter mode | Million instructions/s |
|---|---:|
| Before this change, `DISABLE_TRACE` compiled out | 44.9 |
| Before this change, tracing compiled in but empty observer | 42.8 |
| New `step<false>()` in a trace-capable executable | 43.4 |
| New traced executor, callback only counts records | 40.1 |
| New traced executor, format and retain each instruction | 3.78 |

The first four used 10 million instructions per run; formatting used 2 million.
This shows a small code-generation/runtime-check difference between the
compiled-in and compiled-out paths, not a large optimization collapse.
Formatting/history costs much more, and the frontend's deliberate cap/sleep
dominates even that. Older trace-enabled `printf` logging also incurred
formatting and terminal I/O costs that disappeared when its trace flags were
off; the capped/sleeping TUI scheduler was an additional regression.

To isolate `std::function`, the same formatter/history operation was also called
directly and through `std::function`, in both orders: roughly 4.2 versus 4.4
million records/s. There was no material advantage to replacing the callback
in this workload, so it remains in the traced executor. These optimized
microbenchmarks are not a general claim that type erasure is free.

## Reproduce current modes

From the project root:

```sh
c++ -std=c++17 -Ofast -flto \
  -Isrc/core/cpu -Isrc/core/bus -Isrc/core/iface -Isrc/util -Isrc/ux \
  tools/benchmark_trace.cpp -o /tmp/buddy8800-benchmark
/tmp/buddy8800-benchmark fast 10000000
/tmp/buddy8800-benchmark empty 10000000
/tmp/buddy8800-benchmark count 10000000
/tmp/buddy8800-benchmark format 2000000
/tmp/buddy8800-benchmark direct-format 2000000
/tmp/buddy8800-benchmark function-format 2000000
```

For a current compiled-out comparison, add `-DDISABLE_TRACE` to compilation
and run `empty`. The historical rows above were measured against a temporary
copy of the interpreter taken before this change, using
`-DBUDDY8800_BENCH_BASELINE` (and `-DDISABLE_TRACE` for the compiled-out row).
Performance measurements are intentionally not assertions in the test suite.
