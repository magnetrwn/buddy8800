## Lightweight compatibility cleanup — 2026-09-22

With test changes now in scope, this pass removes the remaining low-risk
compatibility surface before beginning the core contract changes:

- Tests include the current repository-qualified headers and link directly to
  `buddy8800_application`. The `buddylib`/`buddy8800_lib` aggregate and its
  legacy include directories are removed.
- The compatibility headers `card.hpp`, `pty.hpp`, `sysconf.hpp`, `util.hpp`,
  `defines.hpp`, and `ux.hpp` are removed. Callers now include the focused card,
  PTY, configuration, formatting, CPU, and frontend headers they use.
- The legacy `bus_map_s()`, `emulator::info()`, and `emulator::setup()` adapters
  are removed. Presentation and program loading remain in their existing
  dedicated modules. `run_tui()` now lives in `buddy8800::ux` rather than the
  global namespace.
- `cpu_state` keeps its packed register storage private and provides state
  equality for tests. Tests use `trace_history::CAPACITY`, so its lower-case
  compatibility alias is gone.
- Unused numeric type aliases and the old `type_definitions` namespace alias
  are removed. The basic global aliases still used throughout the current core
  remain until a broader namespace/API migration is worthwhile.

This is intentionally the last light cleanup pass. Pseudo-BDOS removal and
conversion from `cpu<address-space>` to a concrete bus CPU remain next, but
they alter diagnostic coverage and execution/fetch code and therefore belong
to the heavier core phase. Dual memory/I/O mappings, explicit bus transactions,
ownership changes, and transport redesign also remain deferred.

Validation: the Debug build and all 13 CTest tests pass. Tests were changed only
to consume the current interfaces; `STYLE.md` remains scoped to `src/` and no
test-only formatting policy was introduced.

## Applied style pass — 2026-09-21

Applied `STYLE.md` across `src/`: unindented namespace bodies with boundary
blank lines, conventional path-derived include guards, separated definitions,
expanded control flow, and Doxygen summaries/groups for the new interfaces.
The PTY implementation and CPU dispatch now show each branch action on its own
line. Project namespaces and implementation targets use `buddy8800`; constants,
enum values, and template parameters follow their documented naming rules.
`src/.clang-format` records the formatting choices (clang-format 22); its scope
excludes `tests/` and external dependencies. No emulator behavior is intentionally
changed, and deprecated diagnostic functionality remains available.

Validation: all 13 existing tests pass in Debug and Release, including Release
with `DISABLE_TRACE=ON`. The TUI smoke checks pass for pause/step/run, reporting,
paging, resize, halt, diagnostic output, signal cleanup and CLI/headless loading.
`tests/` is unchanged. Formatting is checked with clang-format 22.

### Compatibility exceptions and style judgments

- Unchanged tests require the public CPU register/flag spellings (`A`, `PC`,
  `flgC`, the register/flag enums and `set_Z_S_P_flags`). This API family retains
  its existing names. A complete snake_case migration needs a separate API/test
  decision; duplicating every accessor solely for style would add clutter.
- CPU opcode helper methods intentionally retain their established uppercase
  opcode spellings (`NOP`, `LXI`, `MOV_FROM_M`, `RETURN_ON`, and so on). These
  names are a readability aid when following the decoder and are the one
  deliberate exception to the snake_case rule within `cpu.hpp`.
- Named constant values use upper snake case, including `BAD_U8`,
  `FRAME_INTERVAL`, and enum values.
- The specific upper-snake-case include-guard rule takes precedence over the
  general snake_case preprocessor rule. The existing `DISABLE_TRACE` build
  option and legacy `T_ITERATOR_SFINAE` macro (including its `T` parameter
  convention) retain their public spelling. External APIs and assembly mnemonic
  strings keep their original spelling as well.
- Existing braces are retained. Short inline methods remain compact when clear;
  control-flow bodies always start on a subsequent line. Formatting changes do
  not add or remove bus accesses, change evaluation order, or retire diagnostics.

The earlier structural-refactor notes and historical audit below retain their
original naming where they describe that earlier state; this section takes
precedence for current names.

## Applied structural refactor — 2026-09-20

This implementation pass changes `src/` and this document only. `tests/`,
fixtures, ROMs, and the root build configuration remain unchanged. The historical
audit below is retained for context; its original paths/line numbers describe
the source before this pass. This status section takes precedence.

### Completed

| Area | Applied change / where to look |
| --- | --- |
| Cards | `core/bus/card_base.hpp` contains the base contract; `core/cards/data_card.hpp`, `ram_card.hpp`, `rom_card.hpp`, and `serial_card.hpp/.cpp` separate implementations. RAM/ROM remain aliases of the existing template, so their constructor and write-lock contracts do not change. Template implementations stay in headers. |
| Card descriptions | `card_identify` owns its strings and `identify()` is const. Serial identification no longer mutates a cached string or returns pointers into it. `bus::describe()` returns owned snapshots. |
| Machine presentation | `ux/machine_view.hpp/.cpp` formats snapshots for plain output or ncurses. The TUI requests uncoloured text directly; ANSI stripping is gone. Bus routing no longer implements terminal formatting. |
| Configuration | `app/configuration.hpp/.cpp` parses TOML into plain descriptors and resolves config-relative paths without constructing devices. `app/system_config.hpp/.cpp` builds/owns the cards and bus. TOML is no longer exposed through a public header, and callers include the application header directly. |
| Startup and machine | `app/options.*`, `binary_file.*`, and `program_loader.*` separate arguments, binary reading, and loading policy. `app/emulator.*` owns the configured machine and steps it. `main.cpp` composes these. |
| Frontends | `ux/frontend.*` selects interactive/plain operation; `ux/plain.cpp` owns plain output. `ux/tui.cpp` now coordinates commands and execution, with screen rendering/input decoding, terminal lifetime, legacy console formatting, and observer lifetime under `ux/tui/`. |
| Execution scheduling | `app/execution.*` names the frame budgets and extracts instruction batching. The 4,096-instruction/8 ms traced budget, 25 ms traced rest, and 33 ms untraced batches with clock checks every 256 instructions remain intact. |
| Global output side effects | The TUI binds only the machine's deprecated diagnostic printer to a local stream. It never replaces `std::cout.rdbuf()`. A scope guard detaches the trace callback and restores the previous diagnostic stream during normal exit and exception unwinding. Terminal/observer owners cannot be copied. |
| CPU/UI coupling | Register highlighting compares named displayed values instead of indexing the packed register array. The existing state API stays available for tests. Instruction names move unchanged into `core/cpu/opcode_names.hpp`; no decoder/length redesign is included. |
| Trace build boundary | `DISABLE_TRACE` now selects the plain frontend at the frontend target only. CPU definitions are identical across targets. Existing `step<false>()`/`interrupt<false>()` specializations still compile out trace calls and skip trace operand reads; runtime reporting does not switch to an observer-only bypass. |
| Utilities | Formatting, parity, iterator constraints, legacy diagnostic printing, and Linux executable paths have dedicated files. The reserved parity and SFINAE macros are gone, and headers explicitly include what they need. Executable-path lookup grows its buffer rather than silently accepting truncation. |
| Types | Type aliases live under `buddy8800::types`, use `using`, and depend on standard headers. Only aliases used by the current implementation remain. |
| Build boundaries | `src/CMakeLists.txt` defines platform, core, application, presentation, and frontend targets. TOML is an application implementation dependency; ncurses belongs only to the frontend. Tests link the application target directly; frontend clients link `buddy8800_frontend`. |

New non-legacy helper code uses `buddy::app`, `buddy::ux`, `buddy::format`,
etc. Existing public class names remain to avoid a mechanical API rename.
The 88-2SIO card retains its polled register layout and byte transport; unused
serial register/status enums were removed without extending emulated behaviour.

### Deprecation status and compatibility

Pseudo-BDOS, its TOML switch, redirection APIs, and non-bus CPU instantiations
are documented as deprecated in source. They still work for the unchanged
diagnostic and trace tests. Deprecation is expressed with comments/Doxygen,
**not `[[deprecated]]` attributes yet**: the root Release flags include
`-Werror`, so attributes on APIs called by unchanged tests would fail the build.
No warnings were disabled to accommodate this pass.

The diagnostic printer has moved to `legacy/diagnostic_output.hpp`; BDOS traps
and their existing reset behaviour still live in the CPU. The local TUI stream
adapter is transitional, not a new permanent guest-console subsystem. Removing
these features requires a later decision about the diagnostic **and trace**
test dependencies, not simply deleting the diagnostic ROM tests.

Configuration data is parsed before devices are opened, and CLI addresses are
parsed before images are loaded. Successful startup is preserved; when several
inputs are invalid, the first reported error may differ from the former
interleaved parsing/loading implementation. Explicit `--config` and `--help`
also no longer require resolving the default executable-relative config path.

Code using the old standalone benchmark commands must add `-Isrc` for qualified
includes. To measure the no-instrumentation CPU path, use `step<false>()` (the
benchmark's `fast` mode); defining `DISABLE_TRACE` alone no longer changes CPU
template behaviour. Existing CMake frontend selection remains supported.

### Deferred implementation work

These require changes to contracts, emulation behaviour, or regression coverage
beyond this structural pass:

* **Memory and I/O mappings on one card:** replace `is_io()`, make address-space
  selection reach each device access, and update matching, conflict detection,
  descriptions, and config together. Dual mappings remain an accepted design
  target, but are **not implemented**. A boolean description still represents
  today's single-space contract.
* **Explicit bus transactions / narrower card capabilities:** removing proxy
  indexing and changing read-modify-write behaviour affects CPU memory accesses
  and side effects. Preserve current first-reader/broadcast-write overlap,
  forced writes, write locking, and IRQ APIs for now.
* **CPU semantics:** external-fetch guards/cursor behaviour, complete reset
  semantics, interrupt handling, loader bounds/iterator contracts, and a shared
  instruction length/decoder source need dedicated validation. The dispatch
  table and opcode implementations were deliberately left intact.
* **Legacy removal and concrete CPU:** remove pseudo-BDOS and generic CPU
  templates together with their diagnostic-test migration. Compiler deprecation
  attributes are unnecessary if those APIs are removed in that phase.
* **Ownership redesign:** `system_config` remains the machine's device/bus owner;
  moving that ownership into a new machine type would unnecessarily change the
  interface exercised by monitor tests. The bus still borrows card pointers.
* **Transport redesign:** Unix PTY blocking helpers, framing APIs and resource
  management remain as before. A minimal transport interface or alternative
  backend can be developed separately; the serial device continues to call only
  its non-blocking byte operations.
* **Further presentation/controller decomposition:** the TUI state and command
  switch are small local components rather than a new frontend hierarchy.
  A reusable debugger controller or typed trace-event redesign is deferred.
* **Final namespace/API cleanup:** basic global type aliases remain throughout
  the current global core API. Removing them is best done with a broader
  namespace migration rather than replacing every type use with a long
  qualification in otherwise-global classes.

### Validation

* Baseline and refactored Debug builds: all **13 existing CTest tests passed**.
* Release (`-Ofast -Werror -flto`), with ncurses enabled and with
  `DISABLE_TRACE=ON`: all **13 existing tests passed** in each mode. The final
  executable was rebuilt with ncurses enabled.
* Every `src/` header compiled on its own with C++17, warnings-as-errors, and
  only `-Isrc`, checking independence from historical include ordering.
* Additional temporary checks outside the repository exercised owned/const
  device descriptions, plain/ANSI formatting, local diagnostic output binding,
  and restoration/detachment during exception unwinding.
* A temporary PTY-driven smoke check passed in Debug and Release: initial pause,
  stepping, running, reporting toggles, paging/card keys, small-terminal pause
  and resize recovery, HLT staying visible, diagnostic ROM output, quitting,
  SIGINT/SIGTERM restoration, redirected/plain startup, and config-relative
  loading from another working directory. The plain build has no ncurses
  dynamic-library dependency.
* `git diff --check` is clean; `git diff -- tests/` is empty. Tests, their
  fixtures, and their CMake file were not edited.

Build/check directories used: `/tmp/buddy8800-refactor-debug` and
`/tmp/buddy8800-refactor-release`. Temporary smoke-check sources/fixtures are
under `/tmp/buddy8800-refactor-check.8R3gO4`; no new test files were added to the
repository.

## Original audit — scope and conclusion

The original source-only audit covered 16 files under `src/` (3,024 lines).
At that stage no implementation files or `tests/` were changed, and the working
tree was clean before this document was added.

The main problem is not the amount of code in the 8080 interpreter. It is that
machine construction, the execution loop, terminal selection, trace collection,
and ncurses rendering have no stable boundaries. The September 2026 TUI change
put those concerns in `ux.hpp` and one 228-line `run_tui` function. As a result,
adding a frontend or inspecting a simple startup path requires following code
through configuration, CPU hooks, global streams, and terminal state.

Refactor the outer layers first and preserve the CPU's observable behaviour.
The CPU dispatch table should be a later, separately validated change.

## Decisions recorded after review

The following decisions refine the plan and supersede any broader alternatives
below.

* `serial_card` is intentionally a **polled 88-2SIO compatibility subset**. It
  uses the 6850 ACIA's register locations, while deliberately implementing only
  the two-register, polled behaviour needed by the emulator. The refactor must
  preserve that contract; it must not rename the device generically or turn a
  structural change into a full 6850 implementation.
* Pseudo-BDOS is now a deprecated diagnostic-only facility. It is not a
  production machine feature and should be removed, together with its
  configuration key and redirection API, once the related diagnostic test scope
  is retired or migrated. The current suite still uses it in `tests/test_cpu.hpp`,
  so source removal cannot accompany the stated requirement to leave `tests/`
  unchanged. Mark the source API `[[deprecated]]` now and keep only a contained
  compatibility path until that later test decision.
* Generic `cpu<address-space>` support is likewise deprecated. Production code
  should have one CPU bound to the S-100 bus. `tests/test_cpu.hpp` currently
  instantiates `cpu<std::array<u8, 65536>>`, so preserve the template only as a
  temporary test-compatibility implementation; do not generalize its interface.
  A later test-scope change may replace it with a test-memory adapter or harness,
  then remove the specialization path.
* A device may map ranges in memory, I/O, or both address spaces. Replace the
  single `is_io()` classification with explicit address-space mappings; a single
  card can therefore handle the same or different ranges for both spaces.
* Each concrete card derivative belongs in its own focused header/source pair.
  Shared mapping and device contracts belong in common files, not in a growing
  `card.hpp`.

## Target shape

Use dependencies in one direction:

```text
main / command-line parsing
        |
application runtime (constructs machine; selects a frontend)
        |                         \
machine configuration loader         plain frontend / TUI frontend
        |                                      |
machine (CPU + bus + devices) ---- execution/trace/console events
        |
core CPU, bus, card/device, platform transport
```

The machine must not know whether it is being driven by ncurses, a plain
terminal, or a future debugger. The TUI may decide when to run, pause, step,
render, or retain history, but it should receive a narrow machine API and typed
events rather than modify process-wide output.

Suggested public seams:

* `MachineConfig` is data, and `ConfigLoader` turns TOML into that data.
* `Machine` owns the bus, CPU, and devices. It exposes `step`, `run_until`, a
  state snapshot, device descriptions, program loading, and interrupt service.
* `ExecutionController` owns run/pause/step state and batching policy. The
  plain frontend can use the same controller without ncurses.
* `InstructionEvent` values are sent to optional sinks. A temporary legacy
  pseudo-BDOS console adapter may use a `ConsoleEvent`, but it is removed with
  the deprecated facility rather than becoming a permanent CPU service.
* A `DeviceMapping` identifies a device range and `AddressSpace` (`Memory` or
  `Io`). A device may expose more than one mapping, including one in each space.
* `Frontend` is a small interface or pair of independent entry points. It owns
  input, layout, screen lifetime, and presentation-specific history.

These names are illustrative. The important part is ownership and dependency
direction, not adopting a framework or a large inheritance hierarchy.

## Prioritized findings

### P0 — create the outer-layer seams first

| Finding | Evidence | Refactoring outcome |
| --- | --- | --- |
| The TUI is controller, scheduler, input mapper, layout engine, renderer, trace owner, and console adapter at once. | `ux/tui.cpp:97-228` | Split terminal session, layout/panes, input-to-command mapping, and run-loop/controller. Keep `run_tui` as a short composition function. |
| The process-wide `std::cout` buffer is replaced to capture deprecated pseudo-BDOS output. | `ux/tui.cpp:32-55` | First isolate this as a legacy adapter that writes to a frontend-provided sink; then delete it with pseudo-BDOS. Do not redirect a global stream or build a permanent console-output subsystem for a deprecated feature. |
| `emulator` both represents the machine and performs CLI program loading; `terminal_ux` only conditionally routes to one of two frontends. | `ux/ux.hpp:15-107` | Replace both with a machine/application boundary. Move argument parsing and file I/O out of the machine. Give the TUI and plain frontend the same prepared `Machine`. |
| Tracing has two controls entangled with two build modes: a CPU `#ifndef DISABLE_TRACE` and a `step<report_trace>` template. | `cpu/cpu.hpp:84-100,390-398`; `ux/tui.cpp:190-222` | Make trace emission an optional runtime sink with a deliberately cheap disabled path. Keep compile-time elimination only as a documented performance build decision, confined to an instrumentation abstraction. |
| Configuration parsing, file loading, card construction, ownership, and bus insertion are one class. | `util/sysconf.hpp:12-75` | Parse TOML into validated descriptors first, then build devices in a factory/composition step. This makes error reporting and future card types comprehensible without making configuration own the machine. |

The first implementation milestone should compile with unchanged behaviour:
the TUI still starts paused, `x` still disables reporting, redirected standard
I/O still selects the plain frontend, and CLI loads still install their current
reset jump behaviour.

### P1 — clarify core contracts exposed to the outer layers

| Finding | Evidence | Refactoring outcome |
| --- | --- | --- |
| `card` is a wide base class. Every device must implement write locks, forced writes, IRQ delivery, clearing, a single memory/I/O classification, and identification even when those concepts do not apply. | `core/bus/card.hpp:37-115`; placeholder IRQ implementations at `226` and `344` | Give each device one or more explicit `DeviceMapping`s, each with an `AddressSpace` and range. Route `read`/`write` through the selected mapping so a card may serve memory, I/O, or both. Keep reset, setup-only memory loading, interrupt provision, and descriptive metadata as narrow capabilities or explicit operations. |
| `bus` contains both hardware routing and an ANSI-coloured presentation formatter. | `core/bus/bus.hpp:276-300`; ANSI stripping in `ux/tui.cpp:58-77` | Return structured device/map data from the bus or machine. Format it separately for plain output and ncurses. Remove terminal escape parsing from the TUI. |
| The index proxy makes reads, writes, and increments look like normal array expressions despite device side effects. | `core/bus/bus.hpp:49-100,216`; CPU use throughout `core/cpu/cpu.hpp` | Prefer explicit `read_memory`/`write_memory` and `read_io`/`write_io` at the CPU boundary. These select a mapping's address space while allowing the same device to back both spaces. Retain an adapter temporarily if needed, then remove it after call sites are explicit. |
| A data card carries a runtime write-lock and a template parameter expressing the same policy. | `core/bus/card.hpp:136-235` | Use one policy: separate immutable ROM from mutable RAM, or one memory-device type with a clearly named immutable setting. Keep force-loading as a setup-only operation. |
| `serial_card` deliberately implements the polled 88-2SIO subset at 6850-compatible register locations, but its declared 6850 register/status enums are unused. | `core/bus/card.hpp:237-345` | Keep the 88-2SIO/6850-compatible name, two-port mapping, and polled semantics. Document the supported subset beside the device, remove enums that do not describe implemented state, and split this card into its own header/source pair. A full 6850 model is explicitly out of scope. |
| `card_identify` stores borrowed C strings and `serial_card::identify()` mutates a member string to keep `detail.c_str()` alive. It is also non-const. | `core/bus/card.hpp:19-28,301-309` | Return a value-owned, `const` `DeviceDescription` (`std::string` fields, address space, range, slot). Let formatters choose colour and layout. |
| Conflict resolution is split between an insertion check, first-reader-wins routing, and broadcast writes. | `core/bus/bus.hpp:110-132,136-151,177-210` | State and centralize overlap policy. Give a configured mapping an explicit priority/order and describe its read and write behaviour in the API. Preserve the present semantics until a deliberate compatibility decision is made. |

### P1 — untangle the CPU's non-CPU responsibilities

| Finding | Evidence | Refactoring outcome |
| --- | --- | --- |
| The CPU owns pseudo-BDOS policy and an output file/stream helper. | `core/cpu/cpu.hpp:116-150,874-889`; `util/util.hpp:29-70` | Mark this API deprecated now. Isolate it behind a temporary diagnostic compatibility adapter so it no longer shapes normal CPU or TUI design, then remove the trap, configuration field, printer, and legacy tests together in a later test-scope change. |
| Interrupt operand fetching changes mutable CPU-wide function pointers. Restoration is not exception safe and the operand index is stateful. | `core/cpu/cpu.hpp:45-80,411-418,897-905` | Represent external instruction bytes as an explicit, scoped fetch source or guard that restores state in its destructor. Reset the operand cursor at the start of each external instruction. |
| Reset state is dispersed. `clear()` resets registers, `allow_reset_twice`, and `halted`, but not `interrupts_enabled`; other execution settings have separate setters. | `core/cpu/cpu.hpp:863-868,881,909-917` | Define `Cpu::reset()` precisely: architectural state, halted state, interrupt state, trace state, and optional host/test services. Put machine reset policy above it. |
| The public CPU state exposes its packed register array; the TUI relies on its ordering. | `core/cpu/cpu_state.hpp:48-49`; `ux/tui.cpp:172-177` | Keep storage private and offer named state accessors or a dedicated immutable `CpuSnapshot`. UI change highlighting should compare named values. |
| CPU trace extraction peeks operands from the bus while a separate utility owns a large opcode-to-text switch. | `core/cpu/cpu.hpp:88-100`; `util/util.hpp:106-359`; `ux/trace_view.hpp:10-18` | Define one instruction metadata source for opcode length, mnemonic, and rendering. Emit raw `InstructionEvent` values from the CPU; format trace lines outside the core. |
| `cpu` is templated as if it accepts a general address-space abstraction, but it leaks bus-specific behaviour through `if constexpr`, pointer-to-member fetch functions, `write_force`, and I/O operations. | `core/cpu/cpu.hpp:26-31,56-80,332-346,811-823` | Mark non-bus specializations deprecated. Keep the template only until diagnostic tests move off `cpu<std::array<u8, 65536>>`; production becomes a concrete CPU/S-100-bus pairing. Do not design or add a general address-space interface. |

The 920-line dispatch implementation is high-risk, already well covered by the
existing test suite, and should not be mechanically reorganized in the first
pass. Once the interfaces above exist, split it only along behaviour-preserving
boundaries: fetch/decode, ALU/register helpers, control flow/stack, I/O, and
interrupt handling. Preserve its opcode dispatch form if it remains the fastest
and clearest choice.

### P2 — make platform, utility, and build boundaries honest

| Finding | Evidence | Refactoring outcome |
| --- | --- | --- |
| `pty` mixes the non-blocking byte transport used by `serial_card` with blocking convenience I/O, text buffers, echo policy, framing, baud configuration, and break APIs. Most of the latter are unused by `src/`. | `core/iface/unix_pty.hpp:27-193`; only `try_getch`/`try_putch` are used by `serial_card` | Keep a small non-blocking byte-transport interface for the serial device. Put blocking/client convenience APIs in a separate utility if they remain needed. Make the Unix implementation and platform selection explicit. |
| The serial card reports guest clock/divider state, while PTY setup exposes host baud/framing controls that the card intentionally does not use. | `core/bus/card.hpp:301-329`; `core/iface/unix_pty.cpp:82-112` | Separate guest UART configuration from host transport configuration in names and types. This prevents future changes from accidentally treating a PTY speed as emulated UART timing. |
| `pty.hpp` claims to be a portability layer but only includes the Unix implementation. | `core/iface/pty.hpp:1-7` | Either make it an actual platform-selected transport header, or remove the indirection and name the Unix dependency directly until another backend exists. |
| `util` is a catch-all class containing output redirection, Linux executable-path discovery, parity, hex formatting, and instruction metadata. | `util/util.hpp:17-359` | Replace it with small namespaces/files: formatting, instruction metadata, and platform/resource paths. Delete `print_helper` with the deprecated pseudo-BDOS path. |
| Project-wide aliases enter the global namespace, `typedef.hpp` pulls in Unix `ssize_t`, and `defines.hpp` is a global SFINAE macro. | `util/typedef.hpp:4-34`; `util/defines.hpp:4` | Put aliases/constants in a project namespace, use `using` aliases rather than `typedef`, use `std::size_t`/portable signed types at the call site, and replace the macro with a local template constraint/helper. |
| `cpu_state.hpp` defines a reserved `__parity` macro and relies on compiler-specific preprocessor conditions. | `core/cpu/cpu_state.hpp:8-13` | Use a project-named constexpr parity helper or compiler wrapper in one internal header. Never define names beginning with two underscores. |
| Several headers rely on transitive includes. `sysconf.hpp`, for example, uses streams, strings, iterators, and exceptions without declaring all of them directly. | `util/sysconf.hpp:4-9,18-66`; similar broad include leakage in `card.hpp` and `cpu.hpp` | Make every public header self-sufficient and minimize includes with forward declarations where feasible. |
| CMake treats almost all application/core code as headers: `buddylib` contains only the Unix PTY implementation, while include directories are global-like and TUI compilation is attached conditionally to the executable. | `src/CMakeLists.txt:1-27` | Give core, platform, application, and TUI their own targets with scoped include directories and dependencies. The app selects either TUI or plain frontend; the core must not receive ncurses or UI definitions. |

## File-by-file disposition

| File | Disposition |
| --- | --- |
| `src/CMakeLists.txt` | Split the current `buddylib` into coherent core/platform/application/frontend targets; make compile definitions and include paths target-scoped. |
| `src/main.cpp` | Retain as the narrow process boundary: parse options into a value object, establish signal-driven stop state, compose application services, translate errors to exit codes. |
| `src/ux/ux.hpp` | Retire the mixed `emulator`/`terminal_ux` abstraction. Its machine operations become `Machine`; startup and frontend dispatch move to application code. |
| `src/ux/tui.cpp` | Split screen lifetime, rendering/layout, input commands, trace/console presentation, and execution scheduling. It should contain ncurses-specific code only. |
| `src/ux/trace_view.hpp` | Keep the bounded history idea, but move it beside the TUI as a presentation model. Make instruction formatting use shared immutable metadata, not the broad `util` class. |
| `src/util/sysconf.hpp` | Replace `system_config` with a parser/validator and a separate machine builder. Preserve config-relative `load` path handling; deprecate then remove `pseudo_bdos_enabled`. |
| `src/core/cpu/cpu.hpp` | Keep opcode semantics stable initially; extract fetch-source state, trace instrumentation, and loader policy. Deprecate pseudo-BDOS and non-bus template use now, then remove both only with their test-scope migration. |
| `src/core/cpu/cpu_state.hpp` | Encapsulate packed representation, replace the reserved parity macro, and introduce a UI-safe snapshot/comparison API. |
| `src/core/bus/bus.hpp` | Separate routing from map presentation, make overlap/priority semantics explicit, support mappings in memory and/or I/O space per card, and gradually replace proxy indexing with explicit bus operations. |
| `src/core/bus/card.hpp` | Split common device contracts, RAM, ROM, and the polled 88-2SIO card into focused files. A device mapping can declare memory, I/O, or both; retain only serial enums that describe the implemented subset. |
| `src/core/iface/pty.hpp` | Decide whether it is a portability seam; implement backend selection or remove it. |
| `src/core/iface/unix_pty.hpp` | Reduce the device-facing API to transport operations; move unused blocking/text/configuration APIs to a separate client-facing abstraction. |
| `src/core/iface/unix_pty.cpp` | Keep resource acquisition and non-blocking retry/error translation here; add a platform-neutral transport implementation boundary. |
| `src/util/util.hpp` | Delete the catch-all class in favour of focused formatting, opcode metadata, and resource-path modules. |
| `src/util/typedef.hpp` and `src/util/defines.hpp` | Replace global aliases/macros with project-namespaced types and normal C++ templates. |

## Recommended implementation order

1. **Document and freeze current behaviour.** Treat the existing tests as the
   regression baseline; do not edit them in this refactoring start. Record the
   current frontend controls, config-relative paths, reset-vector rules, and
   88-2SIO polled behaviour as acceptance criteria. Record pseudo-BDOS and
   non-bus CPU instantiation as deprecated test-compatibility behaviour.
2. **Introduce application data types without moving behaviour.** Add options,
   configuration descriptors, machine/device descriptions, CPU snapshots, and
   trace/console event types. Adapt existing classes to them before changing
   ownership.
3. **Extract `Machine` and the configuration builder.** Move CLI binary loading
   out of `ux.hpp`; let one composition layer own config parsing, card ownership,
   bus creation, CPU construction, and reset/loading policy.
4. **Extract the execution controller and frontend contract.** Make the present
   plain loop and TUI both call the same stepping/interrupt API. Preserve the
   two existing TUI batch policies as named controller settings rather than
   changing performance characteristics during the split.
5. **Remove normal-path dependence on deprecated diagnostics.** Isolate
   pseudo-BDOS/output redirection behind a legacy adapter, relocate trace history
   and formatting to the presentation layer, and remove the CPU's `DISABLE_TRACE`
   knowledge from normal control flow. Do not remove the legacy implementation
   until its tests are separately retired or migrated.
6. **Refactor bus/device descriptions and device contracts.** Move ANSI output
   out of the bus, split every concrete card into its own files, introduce
   memory/I/O mapping sets per card, then make routing operations explicit.
7. **Clean platform, utility, and build structure.** Reduce PTY surface area,
   namespace the types, remove macros/transitive dependencies, and establish
   targets that match the architectural layers.
8. **Only then consider CPU-internal cleanup.** Guard the external-fetch and
   reset fixes with existing regression coverage, then make small isolated
   interpreter changes. Avoid a wholesale opcode-table rewrite.

## Behaviour and compatibility constraints

The refactor must retain these current contracts unless a later change is
explicitly approved:

* `--config` selects a configuration and card `load` paths resolve relative to
  that configuration file; positional binary paths resolve from the invoking
  process.
* The first positional binary currently installs a jump reset vector only when
  its address is above `0x0002`; config `start_with_pc_at` still sets the PC.
* Interactive execution starts paused; Space, `s`, `x`, `q`, page navigation,
  card scrolling, resize pausing, and the 2,048-line history retain their
  documented effects.
* Headless/redirected standard I/O continues to run the plain frontend without
  ncurses. Signal handling continues to stop the emulator cleanly.
* The PTY remains raw and eight-bit clean, permits reconnects, and continues to
  use non-blocking serial-card transport. Guest UART configuration remains
  guest state until timing/framing emulation is intentionally added.
* Bus overlap, unmapped-read (`0xFF`), force-load, serial polling, and IRQ
  behaviours remain compatible while their APIs are made clearer. A card may
  retain the current mapping in one space or explicitly add mappings in both
  memory and I/O spaces.

## Deprecated compatibility paths

Pseudo-BDOS and non-bus CPU templates are exceptions to the normal
compatibility rule. They remain only because the current test suite uses them:
`tests/test_cpu.hpp` enables and redirects pseudo-BDOS and instantiates
`cpu<std::array<u8, 65536>>`. The immediate source refactor should label the
affected public APIs deprecated and prevent new production callers. A later,
explicit test-scope change can remove those tests or migrate their diagnostic
setup, followed atomically by removal of the source compatibility paths and the
`pseudo_bdos_enabled` TOML setting.

## Deferred items

Do not combine this work with a C++20 migration, a new UI toolkit, a new device
plugin system, a full 6850 timing model, or a wholesale CPU decoder rewrite.
Each would change the risk profile and obscure whether the structural refactor
preserved the emulator's current behaviour.
