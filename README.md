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
