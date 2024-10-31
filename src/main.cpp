#include "ux.hpp"
#include "util.hpp"

int main(int argc, char** argv) {
    terminal_ux ux((util::get_absolute_dir() + "/config.toml").c_str());
    return ux.main(argc, argv);
}
