---
title: TTY
icon: material/monitor
---

## PTY

A PTY is a pseudo TTY.

## PTS

A PTS is the slave side of a PTY. You can often find these in the `/dev/pts/` directory.
VMs launched by libvirt/QEMU often have their consoles exposed here.

## Relevant Links

- https://yakout.io/blog/terminal-under-the-hood/
- https://www.kernel.org/doc/html/v6.2/driver-api/tty/tty_buffer.html#id1
- https://www.kernel.org/doc/html/v6.2/driver-api/tty/tty_ldisc.html
