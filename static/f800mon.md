# ALTMON 1.3 default image

`f800mon.bin` is the project's supplied ALTMON 1.3 image with its command-table
base corrected once. It is 1,031 bytes, loaded at F800–FC06. All bytes are needed;
the final seven bytes are executable code, not padding.

The repair changes only file offset `003B` from `81` to `C1`:

```text
F82F: CALL FBCE                      ; read character
F832: ANI 5F; CPI 'B'; RC; CPI 'U'; RNC
F83A: LXI H,F8C1                     ; was LXI H,F881
      ADD A; ADD L; MOV L,A
      MOV E,M; INX H; MOV D,M; XCHG; PCHL
```

The 19-entry command table starts at F845 for B and ends at F869 for T.
Only L is updated during indexing, so its initial value must be
`(0x45 - 2 * ord('B')) & 0xff = 0xc1`. The former value dispatched B through
F805 instead of F845.

Patched image SHA-256:
`d08af0e8ab3d2e65a1df1e40d1c89b4811d6910eb936cc06891866fd1b39529d`

Builds copy this image unchanged. The monitor regression tests keep an independent
copy under `tests/fixtures/altmon/`, so changing the distributable image does not
silently change the software used by those tests.
