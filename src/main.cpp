#include "cpu.hpp"

int main() {
    cpu processor;
    for (int i = 0; i < 30; i++) // TODO: testing with a bunch of steps
        processor.step();
    return 0;
}
