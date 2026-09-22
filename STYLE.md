# buddy8800 C++ style guide

Use this guide for future source refactors. Apply these rules consistently to
new and touched code; do not mix styles within a file.

## Project name and identifiers

Use `buddy8800` whenever referring to the project in source comments,
documentation, target names, messages, namespaces, and other identifiers. Do
not introduce `buddy` as an abbreviated project name.

Use strict `snake_case` for project-defined functions, variables, types,
namespaces, files, CMake targets, and preprocessor names. Preserve an external
library or system API's spelling when calling it.

Use upper snake case for named constant values. This includes `constexpr`,
`static const`, `static constexpr`, and namespace-scope constant objects, as
well as enum members. Examples include `BAD_U8`, `FRAME_INTERVAL`, and
`command::TOGGLE_RUN`. Do not treat a function-local `const` used to hold an
intermediate result, or immutable per-instance state, as a named constant.

Opcode implementation methods in `src/core/cpu/cpu.hpp` are the deliberate
function-naming exception. Keep their established uppercase mnemonic names,
using upper snake case for compound names: `NOP`, `MOV_FROM_M`, `RETURN_ON`,
and `ALU_IMM`. This keeps the decoder and implementation blocks easy to scan.

## Include guards

Every project header must use a conventional include guard. Do not use
`#pragma once`.

```cpp
#ifndef BUDDY8800_NAME_HPP_
#define BUDDY8800_NAME_HPP_

// Header contents.

#endif
```

The macro must be unique, derived from the repository-relative path, use upper
snake case, and end in `_HPP_` for `.hpp` files or `_H_` for `.h` files. Prepend
`BUDDY8800_` to guard names.

## Namespace blocks

Do not indent content inside a namespace block. Leave one blank line after the
opening namespace line and one blank line before its closing brace.

```cpp
namespace buddy8800::app {

class machine_config {
    // Class members are indented normally.
};

std::string usage();

}
```

The same rule applies to unnamed namespaces.

```cpp
namespace {

void stop(int) {
    stopped = 1;
}

}
```

## Definitions and control flow

Separate every function or class definition in a file with at least one empty
line. This applies to adjacent free functions, methods defined out of line,
classes, structs, and lambdas assigned as named definitions when a blank line
makes the boundary clearer.

Keep every control-flow body on a subsequent line. Do not compress a condition
and its action into one line. Braces are allowed and only preferred if they make
the scope clear, but their placement is not prescribed by this guide.

```cpp
if (result < 0)
    fail("poll");

while (!try_getch(byte))
    wait_for(master_fd, POLLIN);

if (max == 1) {
    data[0] = getch();
    return;
}
```

Avoid compressed constructs such as these:

```cpp
if (result < 0) fail("poll");
while (!try_getch(byte)) wait_for(master_fd, POLLIN);
if (max == 1) { data[0] = getch(); return; }
```

Short methods, however, can be compressed only if well interleaved by an empty
line between entires:

```cpp
/// @brief Assignment operator, writes a byte to the indexed bus location.
inline bus_index_iface& operator=(u8 byte) { bus_ref.write(adr, byte); return *this; }
```

Expand short `if`, `else`, `for`, `while`, `do`, and `switch` branches so a
reader can follow execution one statement per line. In particular, refactor
the dense style in `src/core/iface/unix_pty.cpp` when that file is next touched.

## Code comments and documentation

Make sure to apply Doxygen-style comments in relevant header files. Check the
style in `src/core/bus/card_base.hpp` as reference. Keep the @brief comments
short, only expanding with a longer description within the parent object comment
block, if necessary.

```cpp
/**
 * @brief Base class for all cards.
 *
 * This class is the base class for all cards, it provides a set of methods that should be implemented by all cards that
 * want to interface with the bus class. It also provides some prepared methods that can be used by the bus or the card
 * itself to handle some common operations.
 */
class card {
protected:
    bool write_locked = false;
    bool irq_raised = false;

public:
    /// @name Commonly used methods.
    /// \{

    /// @brief Check if the card is write-locked.
    bool is_w_locked() const { return write_locked; }
```

Produce comment groups like in the example above when linked methods should
be grouped together in documentation.

## Applying this guide

Style changes must preserve behaviour. When reformatting a file, keep the
change focused, build the affected targets, and run the existing tests. Apply
the rules to compatibility code and newly added files as well as to the main
implementation; do not create a second style for legacy wrappers.
