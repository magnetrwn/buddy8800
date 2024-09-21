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
+ **Optional for testing:** Either install Catch2 globally or run `git submodule update --init --recursive` to add it as a git submodule.
+ Run `./build.sh` from the root directory (check the script, there are more options available).
+ The final executable will be placed in the `bin/` directory.

### Resources and Documentation

Here are some of the resources I used to figure out various aspects of this project

**Overview**

+ [Wikipedia/Altair 8800](https://en.wikipedia.org/wiki/Altair_8800), a general overview.

**Intel 8080**

+ [Wikipedia/Intel 8080](https://en.wikipedia.org/wiki/Intel_8080), general Intel 8080 info, with an unexpectedly good layout for opcodes.
+ [Emulator101](http://www.emulator101.com/) and [Emulator101/Opcode List](http://www.emulator101.com/reference/8080-by-opcode.html), a great resource for understanding the basics of emulation, with a comprehensive opcode list.
+ [Emulator101/cpudiag.bin](http://www.emulator101.com/files/cpudiag.bin), a diagnostic program for the Intel 8080, which was used throughout development for testing.
+ [space-invade.rs/cpudiag.lst](https://github.com/cbeust/space-invade.rs/blob/main/emulator/cpudiag.lst), a very useful listing of the assembled `cpudiag.bin` program.
+ [Altair Clone/Programmers Manual](https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf), very useful for understanding how opcodes run and patterns in the instruction set.
+ [Why did CP/M require RAM in the bottom part of the address space](https://retrocomputing.stackexchange.com/questions/6442/why-did-cp-m-require-ram-in-the-bottom-part-of-the-address-space) as well as [Test emulated 8080 CPU without an OS](https://retrocomputing.stackexchange.com/questions/9361/test-emulated-8080-cpu-without-an-os), some good information on CP/M memory maps and execution.
+ [Auxiliary Carry and the Intel 8080's logical instructions](https://retrocomputing.stackexchange.com/questions/14977/auxiliary-carry-and-the-intel-8080s-logical-instructions), a very specific question that fixed diagnostics failing and is not very easy to find.

**Altair 8800**

+ [What additional hardware was required for BASIC on an Altair 8800](https://retrocomputing.stackexchange.com/questions/14675/what-additional-hardware-was-required-for-basic-on-an-altair-8800), a very nice explanation of how Altair 8800 systems originally shipped.

**CP/M and Software**

+ [skx/cpm-dist](https://github.com/skx/cpm-dist), some very cool software for CP/M.
+ [jefftranter/8080](https://github.com/jefftranter/8080), more software, multiple monitor programs.
+ [beriddle/i8080](https://github.com/beriddle/i8080), some demos for the Orion-128 Russian 8080 clone machine, but includes an interesting [16 bit floating point library](https://github.com/beriddle/i8080/tree/master/FP16).
+ [skx/cpmulator](https://github.com/skx/cpmulator/tree/master/ccp), useful CCP sources and binaries.

And most of all, thank you to the **Emulator Development** and **Lazy Developers** Discord servers for all the help and support!

### Screenshots

**First Contact!**

![The first time the 8080 diagnostics ran to the end!](cpu-is-operational.png)
