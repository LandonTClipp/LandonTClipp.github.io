---
date: 2026-11-10
categories:
  - Programming
  - System Design
title: "A Kernel in a Signal Handler: Building a Toy gVisor systrap"
description: How gVisor intercepts syscalls with seccomp, signals, and futexes — explained with a 300-line toy sentry you can compile and run.
---

[gVisor](https://gvisor.dev) runs untrusted containers by putting a *kernel written in Go* — the sentry — between the application and the host. The application is an ordinary, unmodified Linux binary. It executes native instructions directly on the CPU, and yet every system call it makes is handled by the sentry, in a different process, without the application ever noticing. No VM, no binary translation, no recompilation.

The mechanism that pulls this off is called **systrap**, and once you see it, it's delightfully simple: it's a trap vector built out of three commodity Linux features — seccomp, signals, and futexes. This post explains the mental model and then builds a miniature, runnable version: a toy sentry, a toy application, and the signal handler that connects them.

<!-- more -->

## The mental model

A real kernel owns the hardware trap vector: when user code executes `syscall` or touches an unmapped page, the CPU saves state and vectors into the kernel, which handles the event and `iret`s back. gVisor can't own the trap vector — it's a userspace process. So it builds one out of signals:

- **Every syscall becomes a signal.** A seccomp filter with `SECCOMP_RET_TRAP` turns each `syscall` instruction into a synchronous SIGSYS delivered to the thread that executed it.
- **Every page fault already is a signal.** A bad memory access is a synchronous SIGSEGV/SIGBUS.
- **The signal handler is the trap vector.** It saves the interrupted register state, hands the event to the sentry, waits for a verdict, writes the sentry's answer back into the saved state, and returns. The kernel's `sigreturn` then restores that (doctored) state — the application resumes exactly as if the "kernel" had handled its trap.

The processes involved:

```
            the host kernel (the only real kernel here)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      |  delivers SIGSYS / SIGSEGV          ^  futex, write(), ...
      v                                     |
 +---------------------+          +----------------------+
 |  app process        |          |  sentry process      |
 |  ("stub")           |          |  (userspace kernel)  |
 |                     |          |                      |
 |  app code runs      |          |  syscall emulation   |
 |  natively, at full  |          |  memory management   |
 |  speed, until it    |          |  scheduling          |
 |  traps ----> signal |          |                      |
 |         handler <===|==========|===> event loop       |
 +---------------------+  shared  +----------------------+
            |            mailbox +            |
            |             futex               |
            |                                 |
            |        +----------------+       |
            +------> |     memfd      | <-----+
      maps pages of  | ("MemoryFile") |  maps all of it
                     +----------------+
```

Two details of the real design are worth internalizing:

**The app has contexts, not threads.** An application task's CPU state lives in a `thread_context` struct in shared memory. The stub process's threads are interchangeable "CPUs": a task can trap on one stub thread and resume on another, exactly as kernel tasks migrate between physical CPUs. The sentry — one goroutine per task — is the scheduler deciding which context runs where.

**All application memory lives in a sentry-owned memfd.** The app's address space is just mappings of that memfd, installed lazily: the sentry's memory manager decides what backs each page, and faults are how pages get populated. This is also *why* app memory must be in a shared file: when the app calls `write(fd, buf, len)`, the sentry has to read `buf` — it can only do that because it has its own mapping of the same pages.

## The dance

Here is the complete life of one intercepted syscall:

```
   app code              signal handler                sentry
      |                        |                          |
      |  write(1, buf, n)      |                          |
      |--syscall insn--+       |                          |
      |         seccomp:       |                          |
      |         SIGSYS ------> |                          |
      |                        | save regs into ctx       |
      |                        | ctx.state = EVENT        |
      |                        | futex_wake ------------> | wakes
      |                        | futex_wait(...)          | reads ctx: write()
      |                        |                          | performs the write
      |                        |                          | ctx.retval = n
      |                        |                          | ctx.state = REPLY
      |                        | <----------- futex_wake  |
      |                        | saved RAX = ctx.retval   |
      | <----- sigreturn ------|                          |
      | (resumes after the     |                          |
      |  syscall instruction)  |                          |
```

A page fault is the same dance with a different payload: the handler sends the faulting address instead of syscall arguments, the sentry picks backing memory ("here, memfd offset 0x3000"), and the handler installs the mapping before returning — so the faulting instruction re-executes and succeeds. In real gVisor that reply path is `mm.HandleUserFault`: find the vma, get a pma, map it into the stub's address space.

One subtlety that makes syscalls clean: when seccomp raises SIGSYS, the saved program counter *already points past the syscall instruction*. The handler doesn't need to skip anything — set the return-value register and return.

## The toy

The toy keeps the skeleton and drops everything else:

| real systrap                                      | toy                        |
|---------------------------------------------------|----------------------------|
| traps (nearly) every syscall, allowlists its own  | traps `write` and `getpid` |
| full register file + FPU state in `thread_context`| 3 args and a return value  |
| many contexts multiplexed over a stub thread pool | one app, one context       |
| binary-patches hot syscall sites to skip the kernel entirely (usertrap) | nothing |
| handler runs on a per-thread `sigaltstack`        | normal stack               |
| sentry implements most of Linux, in Go            | two syscalls, in C         |

It's one C file, ~300 lines, two processes after a `fork()`:

- a shared **mailbox page** with a futex-backed state machine (`APP → EVENT → REPLY → APP`) — the toy `switch_context`;
- a **memfd** as the sentry's MemoryFile, mapped fully by the sentry;
- the app's heap: a `PROT_NONE` reservation. Nothing is mapped until the app faults and the sentry provides backing memory;
- a **SIGSYS handler** (seccomp trap → ship syscall to sentry → write back the return value);
- a **SIGSEGV handler** (ship fault address → `mmap` the page the sentry chose → re-execute).

The demo app exercises all of it, including two deliberate lessons:

1. `getpid()` returns 1 — the sentry *is* the kernel, and it virtualizes whatever it likes.
2. `write()` from a stack buffer fails with `EFAULT` — the sentry refuses memory it doesn't manage, which is the toy version of why gVisor backs every application page with its own memfd.

### The code

Everything below is one file, [`toy_systrap.c`](https://github.com/LandonTClipp/LandonTClipp.github.io/blob/main/code/gvisor-systrap-toy/toy_systrap.c). Linux only, x86_64 or arm64.

```c
// toy_systrap.c — a miniature gVisor systrap in ~300 lines.
//
// Two processes: a "sentry" (the userspace kernel) and an "app" (the
// sandboxed workload). The app's write() and getpid() syscalls never reach
// the host kernel: seccomp turns them into SIGSYS, a signal handler captures
// the register state into a shared page, a futex hands control to the
// sentry, the sentry emulates the syscall, and the handler resumes the app
// with the sentry's return value. The app's heap is lazily backed the same
// way: it starts as a PROT_NONE reservation, and each SIGSEGV asks the
// sentry to assign a page of its memfd (its "MemoryFile").
//
// Build:  gcc -O2 -Wall -o toy_systrap toy_systrap.c
// Linux only; x86_64 or aarch64.

#define _GNU_SOURCE
#include <errno.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/futex.h>
#include <linux/seccomp.h>
#include <signal.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <unistd.h>

// ---- Per-architecture register access, like sighandler_{amd64,arm64}.c ----
#if defined(__x86_64__)
#define ARCH_AUDIT AUDIT_ARCH_X86_64
#define REG_RET(uc) ((uc)->uc_mcontext.gregs[REG_RAX])
#define REG_A0(uc) ((uc)->uc_mcontext.gregs[REG_RDI])
#define REG_A1(uc) ((uc)->uc_mcontext.gregs[REG_RSI])
#define REG_A2(uc) ((uc)->uc_mcontext.gregs[REG_RDX])
#elif defined(__aarch64__)
#define ARCH_AUDIT AUDIT_ARCH_AARCH64
#define REG_RET(uc) ((uc)->uc_mcontext.regs[0])
#define REG_A0(uc) ((uc)->uc_mcontext.regs[0])
#define REG_A1(uc) ((uc)->uc_mcontext.regs[1])
#define REG_A2(uc) ((uc)->uc_mcontext.regs[2])
#else
#error "x86_64 or aarch64 only"
#endif

// ---- The shared page: a one-slot mailbox between app and sentry ----------
// The real systrap equivalent is struct thread_context in sysmsg.h, which
// holds the full register file and FPU state. Our events are so simple that
// we only ship the syscall arguments / fault address.

enum event { EVENT_SYSCALL = 1, EVENT_FAULT, EVENT_EXIT };

enum state {          // who owns the mailbox
  STATE_APP = 0,      //   app is running, mailbox idle
  STATE_EVENT = 1,    //   app posted an event, sentry's turn
  STATE_REPLY = 2,    //   sentry posted a reply, app's turn
};

struct thread_context {
  _Atomic uint32_t state;  // futex word
  int event;
  int sysno;               // EVENT_SYSCALL: number and arguments
  uint64_t args[3];
  uint64_t fault_addr;     // EVENT_FAULT: faulting address
  int64_t retval;          // reply: syscall result, or memfd offset
};

// ---- Globals, set up before fork() so both processes inherit them --------
#define HEAP_PAGES 4
static struct thread_context *ctx;  // MAP_SHARED: the "sysmsg" page
static uint8_t *app_heap;           // the app's lazily-backed "vma"
static uint8_t *sentry_view;        // the sentry's own mapping of the memfd
static int memfd;                   // the sentry's "MemoryFile"
static long pagesz;

static void futex_wake(_Atomic uint32_t *w) {
  syscall(SYS_futex, w, FUTEX_WAKE, 1, NULL, NULL, 0);
}

static void futex_wait(_Atomic uint32_t *w, uint32_t val) {
  syscall(SYS_futex, w, FUTEX_WAIT, val, NULL, NULL, 0);
}

// ---- App side: the signal handlers ("kernel entry") ----------------------

// post_event_and_wait is our switch_context(): give the mailbox to the
// sentry, sleep until it hands back a reply, take the mailbox back.
static void post_event_and_wait(void) {
  atomic_store(&ctx->state, STATE_EVENT);
  futex_wake(&ctx->state);
  uint32_t s;
  while ((s = atomic_load(&ctx->state)) != STATE_REPLY)
    futex_wait(&ctx->state, s);
  atomic_store(&ctx->state, STATE_APP);
}

// Every trapped syscall lands here as SIGSYS, courtesy of seccomp.
static void sigsys_handler(int signo, siginfo_t *si, void *ucv) {
  ucontext_t *uc = ucv;
  ctx->event = EVENT_SYSCALL;
  ctx->sysno = si->si_syscall;      // seccomp fills this in
  ctx->args[0] = REG_A0(uc);        // "gregs_to_ptregs"
  ctx->args[1] = REG_A1(uc);
  ctx->args[2] = REG_A2(uc);
  post_event_and_wait();
  REG_RET(uc) = ctx->retval;        // "ptregs_to_gregs"
  // For a seccomp trap the saved PC already points after the syscall
  // instruction, so sigreturn resumes the app as if the kernel returned.
}

// Faults on the managed heap land here; anything else is a real crash.
static void sigsegv_handler(int signo, siginfo_t *si, void *ucv) {
  uint64_t addr = (uint64_t)si->si_addr;
  if (addr < (uint64_t)app_heap ||
      addr >= (uint64_t)app_heap + HEAP_PAGES * pagesz) {
    signal(SIGSEGV, SIG_DFL);  // not ours: re-execute and die normally
    return;
  }
  ctx->event = EVENT_FAULT;
  ctx->fault_addr = addr;
  post_event_and_wait();          // the toy HandleUserFault
  // The sentry chose backing memory (a memfd offset). Install the mapping —
  // address-space changes happen *in the app's process*, at the sentry's
  // direction, just like mapASLocked targets the stub's address space.
  uint64_t page = addr & ~(uint64_t)(pagesz - 1);
  mmap((void *)page, pagesz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
       memfd, ctx->retval);
  // Returning re-executes the faulting instruction, which now succeeds.
}

static void install_handlers(void) {
  struct sigaction sa = {.sa_flags = SA_SIGINFO};
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sigsys_handler;
  sigaction(SIGSYS, &sa, NULL);
  sa.sa_sigaction = sigsegv_handler;
  sigaction(SIGSEGV, &sa, NULL);
}

// The seccomp filter: trap write() and getpid(), allow the rest. (Real
// systrap inverts this: it traps everything and allowlists only what the
// stub's own handler code needs.)
static void install_filter(void) {
  struct sock_filter f[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ARCH_AUDIT, 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_write, 1, 0),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getpid, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRAP),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
  };
  struct sock_fprog prog = {.len = sizeof(f) / sizeof(f[0]), .filter = f};
  prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
    perror("seccomp");
    _exit(1);
  }
}

// ---- App side: the "workload" --------------------------------------------
// After install_filter(), stdio is off limits: printf would write() from
// private buffers the sentry can't read. The app can only do raw write()s
// from its managed heap. (This is the toy's version of why gVisor backs all
// application memory with a sentry-owned memfd.)

static int fmt_num(char *dst, long v) {  // minimal itoa, v >= 0
  char tmp[20];
  int n = 0;
  do tmp[n++] = '0' + v % 10, v /= 10;
  while (v);
  for (int i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
  return n;
}

static void app_main(void) {
  // 1. Touch unbacked memory. The strcpy faults on page 0 of the heap; the
  //    sentry assigns backing memory and we resume mid-instruction.
  char *msg = (char *)app_heap;
  strcpy(msg, "hello from the sandbox\n");

  // 2. A syscall. seccomp -> SIGSYS -> sentry performs the write for us.
  write(1, msg, strlen(msg));

  // 3. The sentry virtualizes getpid(): every sandboxed app is pid 1.
  long pid = getpid();
  char *msg2 = (char *)app_heap + pagesz;  // page 1: faults again
  int n = 0;
  const char *s = "the sentry says my pid is ";
  memcpy(msg2, s, strlen(s));
  n = strlen(s);
  n += fmt_num(msg2 + n, pid);
  msg2[n++] = '\n';
  write(1, msg2, n);

  // 4. The sentry refuses to read memory it doesn't manage.
  char stack_buf[] = "this never gets written\n";
  long r = write(1, stack_buf, sizeof(stack_buf));
  if (r < 0) {
    const char *e = "write from private memory failed with EFAULT\n";
    memcpy(msg2, e, strlen(e));
    write(1, msg2, strlen(e));
  }
}

// ---- Sentry side ----------------------------------------------------------

static int64_t sentry_syscall(void) {
  switch (ctx->sysno) {
    case SYS_getpid:
      fprintf(stderr, "[sentry] getpid() -> telling the app it is pid 1\n");
      return 1;
    case SYS_write: {
      int fd = (int)ctx->args[0];
      uint64_t buf = ctx->args[1], len = ctx->args[2];
      uint64_t lo = (uint64_t)app_heap, hi = lo + HEAP_PAGES * pagesz;
      if (fd != 1 || buf < lo || buf + len > hi) {
        fprintf(stderr, "[sentry] write(%d, %#lx, %lu) -> EFAULT (not app "
                        "memory that I manage)\n", fd, buf, len);
        return -EFAULT;
      }
      // Read the app's buffer through the sentry's own memfd mapping: the
      // toy equivalent of the sentry's internal mappings of its MemoryFile.
      fprintf(stderr, "[sentry] write(1, app:%#lx, %lu) -> doing it myself\n",
              buf, len);
      return write(1, sentry_view + (buf - lo), len);
    }
    default:
      return -ENOSYS;
  }
}

static void sentry_run(void) {
  int next_free_page = 0;  // a bump allocator is plenty for a toy
  for (;;) {
    uint32_t s;
    while ((s = atomic_load(&ctx->state)) != STATE_EVENT)
      futex_wait(&ctx->state, s);

    switch (ctx->event) {
      case EVENT_FAULT: {
        int64_t off = (int64_t)next_free_page++ * pagesz;
        fprintf(stderr, "[sentry] fault at %#lx -> backing with memfd page "
                        "%ld (toy HandleUserFault)\n",
                ctx->fault_addr, off / pagesz);
        ctx->retval = off;
        break;
      }
      case EVENT_SYSCALL:
        ctx->retval = sentry_syscall();
        break;
      case EVENT_EXIT:
        fprintf(stderr, "[sentry] app exited; shutting down\n");
        atomic_store(&ctx->state, STATE_REPLY);
        futex_wake(&ctx->state);
        return;
    }
    atomic_store(&ctx->state, STATE_REPLY);
    futex_wake(&ctx->state);
  }
}

// ---- Setup ----------------------------------------------------------------

int main(void) {
  pagesz = sysconf(_SC_PAGESIZE);

  // The mailbox page, shared across fork().
  ctx = mmap(NULL, pagesz, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  // The sentry's "MemoryFile", and its own view of it.
  memfd = memfd_create("app-memory", 0);
  if (memfd < 0 || ftruncate(memfd, HEAP_PAGES * pagesz)) {
    perror("memfd");
    return 1;
  }
  sentry_view = mmap(NULL, HEAP_PAGES * pagesz, PROT_READ | PROT_WRITE,
                     MAP_SHARED, memfd, 0);

  // The app's heap "vma": address space reserved, nothing mapped. Created
  // before fork() so both sides agree on the address.
  app_heap = mmap(NULL, HEAP_PAGES * pagesz, PROT_NONE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  pid_t child = fork();
  if (child == 0) {
    fprintf(stderr, "[app]    starting; my real pid is %d\n", getpid());
    install_handlers();
    install_filter();  // past this point: no stdio, no direct write()
    app_main();
    ctx->event = EVENT_EXIT;
    post_event_and_wait();
    _exit(0);
  }

  sentry_run();
  waitpid(child, NULL, 0);
  return 0;
}
```

### Run it

On Linux:

```
$ gcc -O2 -Wall -o toy_systrap toy_systrap.c && ./toy_systrap
```

On a Mac, in a Linux container (Apple Silicon: use a native arm64 container — Rosetta-emulated amd64 containers don't emulate the seccomp/futex dance faithfully and the toy will hang):

```
$ docker run --rm -v "$PWD":/w -w /w gcc:14 \
    bash -c 'gcc -O2 -Wall -o toy_systrap toy_systrap.c && ./toy_systrap'
```

Output (sentry log on stderr, app output on stdout, so your terminal may interleave them differently):

```
[app]    starting; my real pid is 11
[sentry] fault at 0xffff89624000 -> backing with memfd page 0 (toy HandleUserFault)
[sentry] write(1, app:0xffff89624000, 23) -> doing it myself
hello from the sandbox
[sentry] getpid() -> telling the app it is pid 1
[sentry] fault at 0xffff89625000 -> backing with memfd page 1 (toy HandleUserFault)
[sentry] write(1, app:0xffff89625000, 28) -> doing it myself
the sentry says my pid is 1
[sentry] write(1, 0xffffec9d0408, 25) -> EFAULT (not app memory that I manage)
[sentry] write(1, app:0xffff89625000, 45) -> doing it myself
write from private memory failed with EFAULT
[sentry] app exited; shutting down
```

Read that output against the code once and the whole trick clicks: the app called `strcpy`, `write`, and `getpid` like any program, but its memory was allocated by another process's page-fault service, its output was performed by another process, and its pid is a lie.

## What the real thing adds

The toy is the skeleton. Real systrap (in [`pkg/sentry/platform/systrap/`](https://github.com/google/gvisor/tree/master/pkg/sentry/platform/systrap)) hardens and generalizes every piece:

- **Everything is trapped.** The filter default-traps all syscalls and allowlists only what the handler itself needs; the handler captures the full register file and FPU state (see `__export_sighandler` in [`sysmsg/sighandler_amd64.c`](https://github.com/google/gvisor/blob/master/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c)), on a dedicated `sigaltstack`, with care for races the toy ignores.
- **Contexts are scheduled.** `switch_context` doesn't just reply to one app — it parks the stub thread and hands it whichever ready context the sentry's scheduler picks, so a pool of stub threads multiplexes many tasks.
- **The kernel is skipped entirely when possible.** The SIGSYS path inspects the syscall site and, if it's the common `mov $sysno, %eax; syscall` pattern, binary-patches it into a jump to `__syshandler` — after which that call site traps into the handler *without a signal, without entering the host kernel at all* (`pkg/sentry/usertrap`).
- **The memory manager is real.** The sentry side of the fault path is [`mm.HandleUserFault`](https://github.com/google/gvisor/blob/master/pkg/sentry/mm/syscalls.go): vmas, pmas, copy-on-write, demand paging from files — dispatched from the task run loop in [`kernel/task_run.go`](https://github.com/google/gvisor/blob/master/pkg/sentry/kernel/task_run.go).
- **And the sentry is an entire kernel** — syscalls, a VFS, a network stack — behind that one entry point.

The part I find elegant: there's no magic layer anywhere. The "kernel entry" is `sigaction`, the "context switch" is a futex, the "page tables" are `mmap(MAP_FIXED)` over a memfd, and the application can't tell the difference.
