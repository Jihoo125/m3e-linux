#!/usr/bin/env python3
"""Capture the raw byte stream a real bash+readline PTY produces while typing
'echo input-ok' at harness speed, plus `stty -a` to show the ECHO flag.
Used to isolate terminal-emulator bugs from PTY behaviour."""
import os, pty, select, subprocess, sys, time

out_path = sys.argv[1] if len(sys.argv) > 1 else "/var/tmp/pty-stream.bin"
m, s = pty.openpty()
env = dict(os.environ, TERM="xterm-256color")
p = subprocess.Popen(["/bin/bash", "--norc", "--noprofile", "-i"],
                     stdin=s, stdout=s, stderr=s, env=env, close_fds=True)
os.close(s)
buf = bytearray()

def drain(t=0.3):
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([m], [], [], 0.05)
        if r:
            try:
                buf.extend(os.read(m, 65536))
            except OSError:
                return

drain(0.8)
os.write(m, b"stty -a\r")
drain(0.6)
for ch in "echo input-ok":
    os.write(m, ch.encode())
    time.sleep(0.05)
    drain(0.1)
os.write(m, b"\r")
drain(0.8)
with open(out_path, "wb") as f:
    f.write(buf)
print(repr(bytes(buf[-600:])))
p.terminate()