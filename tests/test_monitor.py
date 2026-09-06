"""Exercise the executable with pinned ALTMON fixtures through a real Linux PTY.

Usage: test_monitor.py EXECUTABLE FIXTURE_CONFIG
Only Python's standard library is required. The emulator's default config and ROM
are deliberately not used, so these regressions also run with other defaults.
"""
import os
import re
import select
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def read_until(fd, marker, timeout=5):
    data = b""
    deadline = time.monotonic() + timeout
    while marker not in data and time.monotonic() < deadline:
        if select.select([fd], [], [], 0.05)[0]:
            part = os.read(fd, 4096)
            if not part:
                break
            data += part
    assert marker in data, (marker, data)
    return data


executable = str(Path(sys.argv[1]).resolve())
monitor_config = str(Path(sys.argv[2]).resolve())
with tempfile.TemporaryDirectory(prefix="buddy-monitor-") as directory:
    process = subprocess.Popen([executable, "--config", monitor_config],
                               cwd=directory, stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    slave = None
    try:
        startup = read_until(process.stdout.fileno(), b"emulator run")
        match = re.search(rb"pty: '([^']+)'", startup)
        assert match, startup
        name = os.fsdecode(match[1])
        slave = os.open(name, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        banner = read_until(slave, b"*")
        assert b"ALTMON 1.3" in banner, banner

        def command(text):
            os.write(slave, text)
            return read_until(slave, b"*")

        # The monitor inserts spaces itself; commands execute without RETURN.
        filled = command(b"K200020035A")
        dumped = command(b"D20002003")
        assert b"5A 5A 5A 5A" in dumped, dumped
        # A second command proves RX data was consumed, not repeatedly returned.
        command(b"K20012002A5")
        dumped = command(b"D20002003")
        assert b"5A A5 A5 5A" in dumped, dumped
        # The overflow beyond 1 KiB includes code used by range operations.
        assert b"FILL" in filled
        os.close(slave)
        slave = os.open(name, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        dumped = command(b"D20002003")
        assert b"5A A5 A5 5A" in dumped, dumped
        command(b"KF800F80000")
        dumped = command(b"DF800F800")
        assert b"3E 03 D3 10" in dumped, dumped
        # Stop even when a large dump saturates an unread PTY output queue.
        os.write(slave, b"D0000FFFF")
        time.sleep(0.1)
        process.send_signal(signal.SIGTERM)
        stdout, stderr = process.communicate(timeout=5)
        assert process.returncode == 0, (stdout, stderr)
        print("ALTMON boot, fill, dump, repeated input, reconnect and SIGTERM: passed")
    finally:
        if slave is not None:
            os.close(slave)
        if process.poll() is None:
            process.kill()
        process.communicate()

    config = Path(directory) / "config.toml"
    rom = Path(directory) / "halt.bin"
    rom.write_bytes(b"\x76")
    valid = ('[emulator]\nstart_with_pc_at = 256\n'
             '[[card]]\ntype = "rom"\nslot = 0\nat = 256\nload = "halt.bin"\n')
    config.write_text(valid)
    result = subprocess.run([executable, "--config", str(config)], cwd="/",
                            capture_output=True, timeout=5)
    assert result.returncode == 0, result.stderr
    for invalid in (valid.replace("slot = 0", "slot = 18"),
                    valid.replace("at = 256", "at = -1"),
                    valid.replace("at = 256", "at = 65536"),
                    valid.replace('load = "halt.bin"', 'range = 65536'),
                    valid.replace("256\n[[card]]", "65536\n[[card]]")):
        config.write_text(invalid)
        result = subprocess.run([executable, "--config", str(config)],
                                capture_output=True, timeout=5)
        assert result.returncode == 1, (invalid, result.stdout, result.stderr)
        assert b"buddy8800:" in result.stderr
    config.write_text(valid)
    for address in ("65536", "-1", "0x100junk"):
        result = subprocess.run([executable, "--config", str(config), str(rom), address],
                                capture_output=True, timeout=5)
        assert result.returncode == 1, (address, result.stderr)
    print("Config-relative ROM loading, headless HLT, config and CLI validation: passed")
