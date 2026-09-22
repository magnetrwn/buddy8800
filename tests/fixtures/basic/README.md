# Pinned 8K BASIC regression fixture

This machine is used only by the front-panel integration test. It is kept
separate from both `static/` and the existing ALTMON 1.3 regression fixture.

- `altmon-1.1.bin`: ALTMON 1.1 at `0xF800`.
- `mits-8k-basic-4.0.bin`: MITS 8K BASIC Rev. 4.0 at `0x0000`.
- `config.toml`: 24 KiB contiguous RAM for BASIC, RAM below ALTMON's `0xC000`
  stack, the 88-2SIO at `0x10`/`0x11`, and fixed front-panel switches at the
  default port `0xFF`. A switch byte of `0x10` selects 88-2SIO port 1 with one
  stop bit for this BASIC version.

Binary SHA-256 checksums:

```text
3c6840d66eb01a8955e400e007408ff6fdfff3e9e3a8db7072bf0563ae60973c  altmon-1.1.bin
dfe4b1576c6ac9fe1a47e9ba0fe697f098209ef8eab61cd54cffc626a84152d3  mits-8k-basic-4.0.bin
```
