# buddy8800 repository guide

This file is the entry point for coding agents and contributors working in this
repository. It describes the current architecture, behavioral contracts, and
validation expectations. It applies to the whole repository.

## Read these sources first

Use the following documents for distinct purposes instead of duplicating their
contents:

1. [README.md](README.md) describes the user-facing emulator, build options,
   command line, configuration, and reference material.
2. [STYLE.md](STYLE.md) is the single source of truth for C++ style in `src/`.
   Add or change style policy there, not in this file. Do not mechanically apply
   its source-formatting rules to `tests/`.
3. [TODO.md](TODO.md) records applied refactors, design decisions, the historical
   audit, and deferred work. Its newest status section takes precedence over
   older audit text and line references.
4. Public header documentation and nearby implementation comments define local
   contracts. Tests provide regression evidence for observable behavior.

When these sources appear inconsistent, verify the current code and tests, fix
stale documentation as part of the change, and preserve behavior unless the
task explicitly changes that behavior.

## Project purpose and supported environment

buddy8800 emulates an Intel 8080 connected to an Altair-style S-100 bus. The
machine supports RAM and ROM cards plus a PTY-backed, polled 88-2SIO serial
subset. It is intended to run Altair monitor software and BASIC, with a machine
model suitable for later expansion. The current project does not emulate disk
hardware needed to boot a complete CP/M system.

The implementation targets C++17 and Linux. It uses CMake, toml11, and optional
ncurses; tests also require Catch2 and Python 3. Both third-party libraries are
Git submodules under `extern/`.

## Repository map and dependency direction

- `src/main.cpp` is the composition root. It parses options, constructs the
  machine, loads programs, and selects a frontend.
- `src/app/` contains configuration data/parsing, device construction and
  ownership, binary loading, the `emulator` machine facade, and execution-frame
  policy.
- `src/core/cpu/` contains the 8080 interpreter, CPU state, and opcode names.
- `src/core/bus/` contains the S-100 bus and card contracts. The bus borrows card
  pointers; it does not own devices.
- `src/core/cards/` contains one focused implementation per card type. Template
  implementations stay in headers when required.
- `src/core/iface/` contains the Unix PTY transport.
- `src/ux/` contains frontend selection, presentation, trace history, and TUI
  controller/rendering code. Terminal and ncurses concerns stay here.
- `src/platform/` and `src/util/` contain small platform and utility facilities.
- `src/legacy/` contains only functionality awaiting removal with the deprecated
  feature that requires it.
- `tests/` contains Catch2 coverage, a Python PTY integration test, diagnostic
  ROMs, and a pinned ALTMON fixture.
- `static/` contains distributable configurations, ROMs, and related notes.
- `tools/` contains development and performance utilities, not runtime code.

Dependencies should point from `main` and frontends through `app` into `core`.
Core code must not depend on ncurses, terminal layout, CLI parsing, or display
formatting. Configuration parsing produces plain descriptors before device
construction opens host resources. Presentation consumes owned snapshots rather
than reaching into mutable card state.

## Current ownership model

`system_config` owns cards with `std::unique_ptr` and owns the bus. It inserts
borrowed card pointers into the bus. `emulator` owns `system_config`, borrows its
bus, and owns the CPU bound to that bus. Frontends borrow the prepared emulator.

The TUI owns its terminal session, screen windows, trace history, and temporary
observers. Cleanup must remain exception safe: terminal settings are restored,
trace callbacks are detached, and borrowed output streams do not outlive their
owners. Do not introduce process-wide stream-buffer replacement for frontend
capture.

## Core behavioral contracts

### Bus and cards

- The current bus has 18 ordered slots.
- Reads return data from the first matching slot. Writes and forced writes are
  broadcast to every matching card. Preserve this asymmetry when collisions are
  explicitly allowed.
- Memory and I/O accesses are currently distinguished by the bus access signal
  and `card::is_io()`. This one-space-per-card contract is scheduled for
  replacement; do not build new abstractions around it.
- Forced writes intentionally bypass ROM write locking and are used by program
  loading. Ordinary guest writes must respect a card's lock.
- Device descriptions own their text and must be safe to obtain without
  servicing device I/O or otherwise mutating a card.
- Each concrete card belongs in its own focused file. Shared contracts belong in
  `core/bus`, not in an umbrella header.

The accepted future direction is that one card may publish multiple mappings in
memory, I/O, or both address spaces. Address-space selection must reach the
specific card access. See `TODO.md` before changing the bus contract.

### CPU

- Preserve Intel 8080 instruction, flag, interrupt, wrapping-address, and reset
  behavior unless a task explicitly targets those semantics.
- Apply the deliberate CPU opcode naming exception documented in `STYLE.md`;
  keep all naming and formatting policy centralized there.
- `cpu_state` keeps packed register storage private; callers use named register,
  flag, and state APIs.
- A trace observer receives the instruction address, up to three raw bytes, and
  instruction size. `step<false>()` and `interrupt<false>()` must avoid trace
  callbacks and trace-only operand reads.
- HLT stops execution but the frontend remains alive so the final state can be
  inspected.

Generic `cpu<address-space>` support and pseudo-BDOS are deprecated diagnostic
facilities. Production uses a CPU on the S-100 bus. Do not add new users or new
features to either deprecated path; their coordinated removal is a deferred
core task described in `TODO.md`.

### Serial and PTY behavior

`serial_card` intentionally implements a small polled 88-2SIO compatibility
subset using the 6850 ACIA register locations:

- the base port reads status and writes control;
- base plus one reads receive data and writes transmit data;
- only the low eight address bits are decoded;
- receive and transmit readiness must be polled;
- receive holds one byte and transmit holds at most one pending byte;
- host backpressure must not block CPU execution;
- guest control writes retain control/divider state but do not pace transport or
  change the host PTY speed;
- UART interrupts, modem signals, detailed framing, and baud timing are not
  emulated.

The PTY transport is raw and eight-bit clean. It preserves startup output and
allows a client to disconnect and reconnect. Card identification may report the
PTY path but must not consume or produce serial data.

### Configuration and loading

- Config `load` paths resolve relative to the configuration file.
- CLI binary paths resolve relative to the caller's working directory.
- An explicit `--config` path and `--help` must not depend on locating the
  executable's default configuration.
- A configured `start_with_pc_at` directly sets PC. It is different from CLI
  reset-vector installation.
- The first CLI binary loaded above address `0x0002` installs the existing reset
  jump; later binaries do not replace it.
- Parse and validate configuration/options before performing avoidable host
  resource or machine mutations.

### Frontends and execution

- An interactive TUI starts paused. Space toggles run/pause, `s` single-steps,
  `x` toggles reporting, `q` quits, Page Up/Page Down browse trace history, and
  Up/Down scroll card information.
- Trace history retains the latest 2,048 lines. Disabling reporting clears old
  history and selects the compile-time-untraced execution path on the same CPU.
- A terminal smaller than 76 columns by 18 rows pauses execution until resized.
- Redirected stdin or stdout selects the plain frontend. A build configured with
  `DISABLE_TRACE=ON` has no ncurses frontend and runs plainly.
- SIGINT and SIGTERM stop execution and must leave terminal state restored.

Execution batching is a UI scheduling policy in `src/app/execution.*`, not guest
CPU timing. Keep serial transport and instruction semantics independent from the
approximately 33 ms frontend frame schedule.

## Build and validation

Initialize dependencies before the first build:

```sh
git submodule update --init --recursive
```

The usual development check is:

```sh
./build.sh -d -T
```

Equivalent direct commands are:

```sh
cmake -S . -B build-linux \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_TESTING=ON \
  -DDISABLE_TRACE=OFF
cmake --build build-linux -j8
ctest --test-dir build-linux --output-on-failure
```

Release uses warnings as errors and link-time optimization. Changes affecting
conditional frontend code should also be checked with `-DDISABLE_TRACE=ON`.
Build outputs are written to the repository `bin/` directory even when the CMake
build directory is elsewhere, so do not build configurations concurrently.

Run the smallest relevant tests while iterating, then the complete CTest suite
before handoff. Use `git diff --check` for text changes. For source formatting,
use the checked-in `src/.clang-format` and verify the requirements in
`STYLE.md`; formatting tools do not replace the project-specific rules.

Tests may be updated when a contract intentionally changes. Keep regression
intent explicit and do not weaken assertions merely to accommodate an
implementation. Test files need to remain readable but are not governed by the
source-only rules in `STYLE.md`.

The ALTMON fixture under `tests/fixtures/altmon/` is deliberately pinned and
independent of the distributable default machine. Do not replace it when adding
or changing boot images unless the task explicitly updates ALTMON regression
coverage. Do not edit generated files under `bin/` or a CMake build directory.

## Change policy and project objectives

- Read the newest status and relevant deferred item in `TODO.md` before starting
  architectural work.
- Prefer focused changes with a clear ownership or dependency improvement.
  Avoid adding generic interfaces without a demonstrated second implementation.
- Preserve observable emulator behavior during structural refactors. When
  behavior changes intentionally, update tests and user documentation together.
- Avoid restoring compatibility umbrellas or broad transitive includes that
  were deliberately removed. Callers should include and link what they use.
- Keep host I/O and rendering outside CPU and bus semantics. Avoid hidden device
  reads in descriptions, traces, or UI refreshes.
- Update the newest status section of `TODO.md` after completing a refactor pass;
  retain useful design history, but correct claims that no longer describe the
  current code.

The next major objectives are tracked in detail in `TODO.md`. In broad order,
they are removal of pseudo-BDOS and generic CPU specializations, explicit
multi-space card mappings, explicit bus transactions and narrower card
capabilities, safer CPU fetch/reset/interrupt contracts, and later ownership or
transport redesign where concrete needs justify it.
