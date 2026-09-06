# Pinned ALTMON regression fixtures

Both `test_monitor.py` and the C++ monitor test use this machine, independently
of `static/` and `bin/`. CMake copies these files to the test build directory;
the Python test receives the config path explicitly and passes `--config` to
the emulator. Python uses only its standard library.

- `config.toml`: PC at F800, 1,031-byte ROM at F800–FC06, serial ports 10/11,
  and 64 KiB of underlying RAM. Pseudo-BDOS is disabled.
- `f800mon.bin`: ALTMON 1.3, copied from the repaired distributable image when
  this fixture was pinned. Offset 003B is C1 rather than 81; see
  [the repair explanation](../../../static/f800mon.md).

ROM SHA-256:
`d08af0e8ab3d2e65a1df1e40d1c89b4811d6910eb936cc06891866fd1b39529d`

Keep this snapshot fixed when adding other boot images or changing the default
machine. Update it deliberately only when updating ALTMON regression coverage.
