# xv6-riscv (Modified)

An extended fork of [MIT xv6-riscv](https://github.com/mit-pdos/xv6-riscv) — a teaching OS for RISC-V — with additional kernel features and test programs.

## Features

| Feature | Description |
|---------|-------------|
| **Shared memory** | `shm_create`, `shm_get`, `shm_close` — map a shared page across processes by key |
| **Mailboxes** | `mbox_create`, `mbox_send`, `mbox_recv` — bounded message queues for IPC |
| **Copy-on-write** | Lazy page copying on fork; writes trigger a private copy |
| **Symbolic links** | `symlink` syscall with `O_NOFOLLOW` support |
| **Large files** | Extended inode/block support for files larger than the original limit |

## Prerequisites

- RISC-V toolchain ([riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain))
- QEMU ≥ 7.2 with `riscv64-softmmu` support

Both must be on your `PATH`.

## Build & Run

```bash
make qemu
```

Exit QEMU with `Ctrl-A` then `X`.

Other useful targets:

```bash
make clean      # remove build artifacts
make qemu-gdb   # start QEMU paused for GDB debugging
```

## Test Programs

| Program | What it tests |
|---------|---------------|
| `shmtest` | Shared memory create/get/close across processes |
| `mbtest` | Mailbox send/receive |
| `cowtest` | Copy-on-write after fork |
| `symlinktest` | Symlink creation, resolution, and `O_NOFOLLOW` |
| `bigfile` | Large file read/write |
| `master` / `process` | Multi-process coordination demos |

Inside xv6:

```
$ shmtest
$ mbtest
$ cowtest
$ symlinktest
$ bigfile
```

## Project Layout

```
kernel/     Kernel source (VM, proc, fs, shm, mbox, …)
user/       User programs and libc stubs
mkfs/       File-system image builder
```

## Acknowledgments

Based on [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) by Frans Kaashoek, Robert Morris, and contributors. xv6 is a re-implementation of Unix v6 for modern RISC-V multiprocessors, used in MIT's [6.1810](https://pdos.csail.mit.edu/6.1810/) course.

See [LICENSE](LICENSE) for the original license.
