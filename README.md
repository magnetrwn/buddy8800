# Buddy8800 -- An Altair 8800 S-100 Emulator

**An Intel 8080 emulated on an S-100 bus with an Intel 8251 serial expansion card. Designed for running BASIC and CP/M over a PTY.**

<img src=altair-wikipedia-public-domain.webp width=480>

### Getting Started

+ [Building](#building)
+ [Resources and Documentation](#resources-and-documentation)
+ [Screenshots](#screenshots)

### Building

+ Make sure you have installed CMake on your system.
+ Clone the repository and navigate to the root directory.
+ Either install Catch2 globally or run `git submodule update --init --recursive` to add it as a git submodule.
  + Skip this step if you don't want to run tests.
+ Check the `CMakeLists.txt` file to select the desired build options:
  + `CMAKE_BUILD_TYPE` can be set to `Debug` or `Release`.
  + `ENABLE_TESTS` can be set to `ON` or `OFF` (will generate `bin/tests` for CTest).
  + `ENABLE_TRACE` can be defined to enable CPU trace logging source.
+ Run `./build.sh` from the root directory.
+ The final executable will be placed in the `bin/` directory.

### Resources and Documentation

Here are some of the resources I used to figure out various aspects of this project

+ [Wikipedia/Altair 8800](https://en.wikipedia.org/wiki/Altair_8800), a general overview.
+ [Wikipedia/Intel 8080](https://en.wikipedia.org/wiki/Intel_8080), general Intel 8080 info, with an unexpectedly good layout for opcodes.
+ [Emulator101](http://www.emulator101.com/) and [Emulator101/Opcode List](http://www.emulator101.com/reference/8080-by-opcode.html), a great resource for understanding the basics of emulation, with a comprehensive opcode list.
+ [Emulator101/cpudiag.bin](http://www.emulator101.com/files/cpudiag.bin), a diagnostic program for the Intel 8080, which was used throughout development for testing.
+ [space-invade.rs/cpudiag.lst](https://github.com/cbeust/space-invade.rs/blob/main/emulator/cpudiag.lst), a very useful listing of the assembled `cpudiag.bin` program.
+ [Altair Clone/Programmers Manual](https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf), very useful for understanding how opcodes run and patterns in the instruction set.
+ [Why did CP/M require RAM in the bottom part of the address space](https://retrocomputing.stackexchange.com/questions/6442/why-did-cp-m-require-ram-in-the-bottom-part-of-the-address-space) as well as [Test emulated 8080 CPU without an OS](https://retrocomputing.stackexchange.com/questions/9361/test-emulated-8080-cpu-without-an-os), some good information on CP/M memory maps and execution.
+ [skx/cpm-dist](https://github.com/skx/cpm-dist), some very cool software for CP/M.

### Screenshots

**First Contact!**

![The first time the 8080 diagnostics ran to the end!](cpu-is-operational.png)
