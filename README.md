# Buddy8800 -- An Altair 8800 S-100 Emulator

**An Intel 8080 emulated on an S-100 bus with an Intel 8251 serial expansion card. Designed for running BASIC and CP/M over a PTY.**

<img src=altair-wikipedia-public-domain.webp width=480>

### Getting Started

**Markdown jumps to other sections go here.**

### Building

+ Make sure you have installed CMake on your system.
+ Clone the repository and navigate to the root directory.
+ Either install Catch2 globally or run `git submodule update --init --recursive` to add it as a git submodule.
+ Run `./build.sh` from the root directory.
+ The final executable will be placed in the `bin/` directory.

### Referenced Documentation

Here are some of the resources I used to figure out various aspects of this project

+ [Wikipedia/Altair 8800](https://en.wikipedia.org/wiki/Altair_8800), a general overview.
+ [Wikipedia/Intel 8080](https://en.wikipedia.org/wiki/Intel_8080), general Intel 8080 info, with an unexpectedly good layout for opcodes.
+ [Emulator101](http://www.emulator101.com/) and [Emulator101/Opcode List](http://www.emulator101.com/reference/8080-by-opcode.html), a great resource for understanding the basics of emulation, with a comprehensive opcode list.
+ [Altair Clone/Programmers Manual](https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf), very useful for understanding how opcodes run and patterns in the instruction set.
