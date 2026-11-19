---
date: 2026-11-10
categories:
  - Programming
  - System Design
title: "A Kernel in a Signal Handler: Building a Toy gVisor systrap"
description: How gVisor intercepts syscalls with seccomp, signals, and futexes, explained with a 300-line toy sentry you can compile and run.
---

Here is a magic trick. Take an ordinary, unmodified Linux program and run it. Its instructions execute natively on the CPU, at full speed. And yet the Linux kernel never hears from it: every syscall it makes (every `read`, every `write`, every `getpid`) is intercepted and answered by a *different process*, a kernel written in Go running entirely in userspace, which decides what its files contain, what memory it may touch, and even what its pid is. The program cannot tell the difference.

This is gVisor's **systrap** platform, and the trick is assembled from three commodity Linux primitives you have probably used separately: seccomp, signals, and futexes. No hypervisor, no hardware virtualization support, no binary translation. Just a signal handler disciplined enough to impersonate a hardware trap vector, the mechanism a CPU uses to jump into the kernel when a program makes a syscall or touches memory it shouldn't.

This post pulls systrap apart piece by piece (the seccomp filter, the signal handler injected into the guest, the shared-memory handoff, the scheduler, the fastpaths, the live binary patching), with pointers into the actual gVisor source for every claim. At the end, we rebuild the skeleton as a ~300-line C program you can compile and run: a toy sentry that demand-pages its own app's heap and lies to it about its pid.

<!-- more -->

## Virtualization without the virtual machine

When most people speak of virtualization, the first image that often comes to mind is that of a Virtual Machine (VM). VMs emulate many parts of a physical computer: CPUs, memory, devices, PCIe buses, network interfaces etc. What you provide to a VM are the essentials of booting any computer: a kernel (Linux, Windows etc), a rootfs, an initrd, and possibly external storage mediums. Given these basic components, the VM will instantiate a virtual computer and hand over control to the Linux kernel which then performs its own boot process.

There is, of course, an alternative to a fully virtualized computer. That alternative is called gVisor. gVisor is not a full virtual machine. Instead of booting a Linux kernel, it instead acts as the kernel itself to the guest process running underneath it. A traditional VM's model looks like this:

```title="Virtual Machine"
+--------------------------------------------------------------------------------+
| +----------------+  +----------------+  +----------------+  +----------------+ |
| |                |  |                |  |                |  |                | |
| |  PCIe Bus      |  |  Network Cards |  |      CPUs      |  |     UEFI       | |
| |                |  |                |  |                |  |  (firmware)    | |
| |                |  |                |  |                |  |                | |
| +------+---------+  +--------+-------+  +--------+-------+  +-------+--------+ |
|        |                     |                   |                  |          |
| +------v---------------------v-------------------v------------------v--------+ |
| |                                                                            | |
| |                                                                            | |
| |                          Linux Kernel                                      | |
| |                                                                            | |
| +-------^-------------------^-------------------^-------------------^--------+ |
|         |                   |                   |                   |          |
|         |read(2)            |write(2)           |getpid(2)          |open(2)   |
|         |                   |                   |                   |          |
| +-------+--------+  +-------+--------+  +-------+--------+  +-------+--------+ |
| |                |  |                |  |                |  |                | |
| |   Process      |  |   Process      |  |   Process      |  |   Process      | |
| |                |  |                |  |                |  |                | |
| |                |  |                |  |                |  |                | |
| +----------------+  +----------------+  +----------------+  +----------------+ |
+--------------------------------------------------------------------------------+
```

Processes live under the purview of a privileged piece of software (the kernel) that mediates access to emulated virtual resources. The diagram above is of course slightly unilluminating: the virtual resources described above are often indistinguishable by the kernel from real resources. If we zoom out, we see that the VM is itself just an ordinary process running underneath another kernel (the host):

```title="VM-ception"
+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
|                                         +----------------+  +----------------+  +----------------+  +----------------+                                             |
|                                         |                |  |                |  |                |  |                |                                             |
|                                         |  PCIe Bus      |  |  Network Cards |  |      CPUs      |  |     UEFI       |                                             |
|                                         |                |  |                |  |                |  |  (firmware)    |                                             |
|                                         |                |  |                |  |                |  |                |                                             |
|                                         +------+---------+  +-------+--------+  +-------+--------+  +-------+--------+                                             |
|                                                |                    |                   |                   |                                                      |
|+-----------------------------------------------v--------------------v-------------------v-------------------v-----------------------------------------------------+|
||                                                                                                                                                                  ||
||                                                                                                                                                                  ||
||                                                                             Linux Kernel                                                                         ||
||                                                                                                                                                                  ||
|+--------------------------------------^---------------------------------------------------------------------------------^-----------------------------------------+|
|                                       |                                                                                 |                                          |
|                                       |                                                                                 |                                          |
|                                       |read(2)                                                                          |write(2)                                  |
|                                       |                                                                                 |                                          |
|+--------------------------------------+-----------------------------------------++--------------------------------------+-----------------------------------------+|
|| +----------------+  +----------------+  +----------------+  +----------------+ || +----------------+  +----------------+  +----------------+  +----------------+ ||
|| |                |  |                |  |                |  |                | || |                |  |                |  |                |  |                | ||
|| |  PCIe Bus      |  |  Network Cards |  |      CPUs      |  |     UEFI       | || |  PCIe Bus      |  |  Network Cards |  |      CPUs      |  |     UEFI       | ||
|| |                |  |                |  |                |  |  (firmware)    | || |                |  |                |  |                |  |  (firmware)    | ||
|| |                |  |                |  |                |  |                | || |                |  |                |  |                |  |                | ||
|| +------+---------+  +--------+-------+  +--------+-------+  +-------+--------+ || +------+---------+  +--------+-------+  +--------+-------+  +-------+--------+ ||
||        |                     |                   |                  |          ||        |                     |                   |                  |          ||
|| +------v---------------------v-------------------v------------------v--------+ || +------v---------------------v-------------------v------------------v--------+ ||
|| |                                                                            | || |                                                                            | ||
|| |                                                                            | || |                                                                            | ||
|| |                          Linux Kernel                                      | || |                          Linux Kernel                                      | ||
|| |                                                                            | || |                                                                            | ||
|| +-------^-------------------^-------------------^-------------------^--------+ || +-------^-------------------^-------------------^-------------------^--------+ ||
||         |                   |                   |                   |          ||         |                   |                   |                   |          ||
||         |read(2)            |write(2)           |getpid(2)          |open(2)   ||         |read(2)            |write(2)           |getpid(2)          |open(2)   ||
||         |                   |                   |                   |          ||         |                   |                   |                   |          ||
|| +-------+--------+  +-------+--------+  +-------+--------+  +-------+--------+ || +-------+--------+  +-------+--------+  +-------+--------+  +-------+--------+ ||
|| |                |  |                |  |                |  |                | || |                |  |                |  |                |  |                | ||
|| |   Process      |  |   Process      |  |   Process      |  |   Process      | || |   Process      |  |   Process      |  |   Process      |  |   Process      | ||
|| |                |  |                |  |                |  |                | || |                |  |                |  |                |  |                | ||
|| |                |  |                |  |                |  |                | || |                |  |                |  |                |  |                | ||
|| +----------------+  +----------------+  +----------------+  +----------------+ || +----------------+  +----------------+  +----------------+  +----------------+ ||
|+--------------------------------------------------------------------------------++--------------------------------------------------------------------------------+|
+--------------------------------------------------------------------------------------------------------------------------------------------------------------------+
```

We now get into the amusing realization that VMs themselves can themselves host other VMs. There is no theoretical limit to how far down the turtles go, although the practical realities of physics tends to tax each layer with an increasing performance penalty.

gVisor is a completely different kind of virtualization. As you might have noticed, the interface between a process and its kernel is a syscall (of which [there are many](https://www.chromium.org/chromium-os/developer-library/reference/linux-constants/syscalls/#x86_64-64-bit)): a request like "read this file descriptor" or "map me some memory," made by executing a special instruction that hands control to the kernel. Instead of running a full virtual machine that emulates hardware and runs a full Linux kernel, what if we implemented a kernel in userspace that serves each of these syscalls to the processes running underneath?

## The life of a syscall

We need some method for one userspace process to intercept the syscalls of another userspace process such that this application serves the syscall directly. The gVisor model looks something like this:

```title="gVisor"
+--------------------------------------------------------------------------------+
| +----------------+  +----------------+  +----------------+  +----------------+ |
| |                |  |                |  |                |  |                | |
| |  PCIe Bus      |  |  Network Cards |  |      CPUs      |  |     UEFI       | |
| |                |  |                |  |                |  |  (firmware)    | |
| |                |  |                |  |                |  |                | |
| +------+---------+  +--------+-------+  +--------+-------+  +-------+--------+ |
|        |                     |                   |                  |          |
| +------v---------------------v-------------------v------------------v--------+ |
| |                                Linux Kernel         +--------------------+ | |
| |                                                     |  seccomp bpf filter| | |
| |                                                     +-+----------^-------+ | |
| |                                                       |          |         | |
| +-------+-----------------------------------------^-----+----------+---------+ |
|         |                                         |     |          |           |
|         |4. FUTEX_WAKE                    3. futex|     |2. SIGSYS |1. syscall |
|         |                                         |     |          |           |
| +-------v----------------------------+ +----------+-----+----------+--------+  |
| |                                    | |          |     |                   |  |
| |                                    | |         ++-----v----------+        |  |
| |                                    | |         | sysmsg handler  |        |  |
| |                                    | |         |                 |        |  |
| |                                    | |         +-----------------+        |  |
| |                                    | |                                    |  |
| |           Process                  | |            Process                 |  |
| |          gVisor sentry             | |    gVisor stub. Where guest runs.  |  |
| +------------------------------------+ +------------------------------------+  |
+--------------------------------------------------------------------------------+
```

The gVisor sentry and the "guest" process itself are two ordinary processes. There is no virtualization happening in the traditional sense. The syscall flow operates like this:

1. The gVisor stub process, where the guest code actually runs, issues a syscall to the kernel. Before dispatching the syscall, the kernel evaluates the seccomp BPF filter[^seccomp] attached to the thread (installed by the sentry systrap platform at stub setup). The filter returns `SECCOMP_RET_TRAP`, so the kernel never executes the syscall.
2. The kernel synthesizes a SIGSYS signal[^signals] and delivers it synchronously to the thread that made the syscall. Inside of the stub process is a piece of code, the sysmsg handler, that was pre-mapped into the stub process and wired to listen to these signals sent by the kernel. Read that last sentence again: the sysmsg handler was _mapped into the stub process's memory mappings_ as a runnable piece of code by systrap. The sysmsg handler also has shared memory with the sentry, namely the `thread_context` struct. It updates the `thread_context` struct with the entire register file (the complete set of CPU register values) of the application. The arguments to the syscall implicitly reside in the `rdi`, `rsi`, `rdx`, `r10`, `r8`, and `r9` registers per the ABI (the calling convention that dictates which registers carry which syscall arguments). Because this shared struct resides in shared memory, an update by the sysmsg handler is immediately seen by the sentry.
3. After updating `thread_context` with the arguments to the syscall, the sysmsg handler first flips `ctx->state` to `CONTEXT_STATE_SYSCALL`, and then makes one `futex(FUTEX_WAKE)` syscall on that state word.[^futex]
4. The gVisor sentry thread was previously parked in `futex(FUTEX_WAIT)` on that same state word, so the handler's `FUTEX_WAKE` rouses it. The sentry then reads `thread_context`, parses the arguments, and does whatever action that syscall necessitates.[^origrax] The result is then stored in the same `thread_context` struct in the same fields the arguments arrived in. The sentry then enqueues the `thread_context` ID into a ring buffer shared with the stub process. One of the stub threads will eventually asynchronously pop this ID off of the ring buffer, place the results posted to `thread_context` into the `rax` register, and return control back to the application.

A simpler conceptualization of this model can be seen below:

```title="gvisor simplified"

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

## The MemoryFile

The `memfd` is the keystone of the whole design. At startup, the sentry creates a single memory file descriptor via `memfd_create` (a Linux syscall that creates an anonymous, memory-backed file: it has a size and a file descriptor and can be mapped like any file, but it lives purely in RAM and has no name on any disk), which is wrapped by an allocator called [`pgalloc.MemoryFile`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/pgalloc/pgalloc.go). Every piece of shared memory we've discussed so far (the `thread_context` structs, the per-thread sysmsg stacks, the ring buffer of runnable contexts known as the context queue, and, crucially, _all application memory_) is allocated as a range of this one file. The sentry maps these ranges into its own address space with ordinary `mmap(MAP_SHARED)` calls,[^mmap] and it installs mappings of the same ranges into the stub's address space (`MAP_SHARED | MAP_FILE | MAP_FIXED`). You can watch both halves of this dance for the context queue and `thread_context` regions in [`mapSharedRegions`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L435-L492). Two views, one backing store: a write on one side is immediately visible to the other, and, per the futex key mechanics described in an earlier footnote,[^futex] a futex wait on one side can be woken from the other.

Why must _application_ memory live in this file, and not just the communication structures? Because the sentry is the kernel, and kernels dereference user pointers. When the app calls `write(1, buf, 23)`, the only thing that travels to the sentry is the register file; `buf` is just a number that happened to be sitting in `rsi`. To actually perform the write, the sentry must read the 23 bytes at that address _in the stub's address space_. It can do this only because it decided what backs every page of the app in the first place: it knows that the app's virtual address `buf` is backed by, say, offset `0x3000` of the MemoryFile, and it has its own mapping of that same range. The sentry's equivalent of Linux's `copy_from_user` (the helper the real kernel uses to read a buffer out of user memory) is an ordinary memory load through its own view of the MemoryFile.

## The stub process

So far, we have glossed over how the stub itself comes into existence. There is no stub binary on disk and no `execve`. The stub's code (the sysmsg signal handler, the context-switching logic, a few syscall trampolines) is compiled into gVisor as a self-contained blob of position-independent machine code, meaning code that runs correctly no matter which address it is loaded at (C and assembly from [`systrap/sysmsg/`](https://github.com/google/gvisor/tree/release-20261116.0/pkg/sentry/platform/systrap/sysmsg), built with no libc and no relocations the loader would have to fix up). To create a stub, the sentry clones itself, and the child immediately unmaps _everything_ it inherited except this blob ([`unmap()` in `subprocess.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L512)), which has been copied to a randomized address near the top of the address space ([`maxRandomOffsetOfStubAddress`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L54-L66); the randomization range is fixed rather than host-dependent, partly so that the layout survives checkpoint/restore). What remains is a nearly empty husk of a process: no libc, no heap, no Go runtime. Just a few pages of hand-written trap-handling code and a vast, empty address space that the sentry now fully controls. One such subprocess exists per guest address space (i.e., per guest process), and each one contains two very different kinds of threads.

The first is the **syscall thread**, and it is a puppet. The sentry keeps it permanently ptrace-stopped[^ptrace] and uses it to execute syscalls _inside_ the stub's address space: write the desired syscall number and arguments into its registers with `PTRACE_SETREGS`, let it execute one `syscall` instruction, read the result back out. This is the sentry's remote-execution gadget, and it answers a question that should have been nagging you since the memfd section: how does the sentry install mappings into an address space it doesn't own? `mmap` affects the calling process, and the sentry is the wrong process. The answer is that the sentry makes the stub call `mmap` on itself, by puppeteering the syscall thread ([`thread.syscall()` in `subprocess.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L781-L818): `PTRACE_SETREGS`, resume, wait for the syscall-exit stop, read `rax`). Every page the application will ever touch is mapped this way.

The second kind is the **sysmsg threads**: the "CPUs" where guest code actually runs. They are created on demand (`clone` with `CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_PTRACE`; you can read the exact flag set in the stub's own seccomp allowlist, which [permits precisely that clone and no other](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stubfilter/stubfilter.go#L73-L85)), and the sentry uses ptrace exactly once, at birth, to dress them. The whole ritual is one function, [`createSysmsgThreadLocked`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L1143): allocate an 8-page per-thread region from the MemoryFile and map it into both processes, point the thread's `sigaltstack` into that shared region (`sigaltstack` designates an alternate stack that the kernel switches to whenever it runs this thread's signal handlers), register the sysmsg handler for `SIGSYS`, `SIGSEGV`, `SIGBUS`, `SIGFPE`, `SIGTRAP`, and `SIGILL`, and install the seccomp filter.[^rostack] Then comes my favorite detail of the whole initialization sequence: the sentry starts the thread by [_sending it a fake SIGSEGV_](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L1283-L1288) (the source comment deadpans: "Send a fake event to stop the BPF process so that it enters the sighandler"). The thread falls into the signal handler for the first time, [notices it is in an initialization state](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L250-L254), and parks waiting for work. The sentry detaches, and ptrace is never used on this thread again. For the rest of its life, it bounces between guest code and the signal handler, communicating with the sentry purely through shared memory, futexes, and the occasional interrupt signal.

The decision to place the signal stack in shared memory is more important than it first appears. When the kernel delivers a signal, it writes the complete CPU state of the interrupted thread (every general-purpose register, plus the full floating-point and vector state[^xsave]) onto the signal stack as the signal frame. Because that stack is a mapping of the MemoryFile, the kernel has unwittingly serialized the guest's CPU state _directly into memory the sentry can see_. The sentry can read and even modify the suspended thread's registers as plain data, no ptrace required.

## The seccomp filter

The filter gVisor installs on sysmsg threads is the mirror image of a normal seccomp sandbox. A typical sandbox allows most syscalls and blocks a dangerous few; systrap's filter sets the _default_ action to `SECCOMP_RET_TRAP` and allowlists almost nothing. The entire allowlist is the [`SysmsgThread` program in `stubfilter/stubfilter.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stubfilter/stubfilter.go#L163-L216) (note the `DefaultAction: seccomp.Trap` at the bottom), and it is exactly what the sysmsg handler itself needs to function: [`futex`, restricted to `FUTEX_WAIT` and `FUTEX_WAKE` with constrained arguments](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stubfilter/stubfilter.go#L168-L187), `rt_sigreturn` (the syscall a signal handler returns through, telling the kernel to restore the saved CPU state from the signal frame), `sched_yield` (voluntarily give up the CPU), and on x86, [`arch_prctl` for reading and writing the FS/GS base registers](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stubfilter/stubfilter_amd64.go#L58).

But wait: the handler and the application run on _the same thread_. Seccomp filters are per-thread and can never be removed (the filter is installed under [`PR_SET_NO_NEW_PRIVS`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L1257-L1275), a one-way "never gain privileges again" flag that the kernel demands before an unprivileged process may impose a filter, driven remotely through the still-ptraced thread; the BPF program itself was staged in stub memory and [`mprotect`ed read-only](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stub_unsafe.go#L96-L106) beforehand). So how can `futex` be forbidden to the app but allowed to the handler? The answer is that `seccomp_data`, the structure a BPF filter gets to inspect, includes the **instruction pointer** of the syscall: the address of the very instruction that invoked it. Every rule in the allowlist requires that the calling instruction live above the start of the stub's code region (in the filter source, that's the [`seccomp.GreaterThan(stubSysmsgStart)` constraint on the `// rip` position of every single rule](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/stubfilter/stubfilter.go#L168-L206)), and for `futex`, that the futex _address_ point into the shared region too. The same `futex` syscall is allowed when issued from the trampoline code the sentry injected, and trapped when issued from anywhere else. The filter is partitioning the address space into "kernel text" and "user text."

The mechanics of `SECCOMP_RET_TRAP` itself are worth spelling out precisely, because the whole design leans on them:

1. The kernel evaluates the filter at syscall entry, _before_ dispatching to the syscall implementation. On a trap verdict, the syscall simply never executes.
2. The kernel synthesizes a `SIGSYS` and force-delivers it, synchronously, to the offending thread. The `siginfo_t` comes pre-populated with the syscall number (`si_syscall`), the ABI in use (`si_arch`), and the address of the syscall instruction (`si_call_addr`); the handler doesn't need to dig any of this out of the registers.
3. The program counter saved in the signal frame already points to the instruction _after_ `syscall`. When the handler eventually returns and `rt_sigreturn` restores the frame, execution resumes exactly where a real kernel would have resumed it. The handler only needs to place the return value in `rax`.

One more wrinkle for completeness: on kernels 5.11 and newer, systrap also enables **syscall user dispatch** ([`PR_SET_SYSCALL_USER_DISPATCH`, enabled per-thread at clone time](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L301-L320)), a mechanism purpose-built for exactly this interception pattern. It delivers the same `SIGSYS` for any syscall issued outside a designated code region (the stub text), without evaluating a BPF program, and the comment at that call site notes it is "generally more efficient than seccomp." The seccomp filter is still installed either way; on old kernels it is the trap mechanism, on new ones it is the backstop.

## The sysmsg handler

Everything the handler does happens under hostile conditions: it runs in the guest's address space, interleaved with guest execution, on a thread whose FS register, the register through which x86 code finds its thread-local storage, belongs to the _guest_.[^tls] It cannot use thread-local variables, globals are shared with code it doesn't trust, and it can't even call `getpid` to figure out who it is, because syscalls trap. So how does the handler find its own per-thread state? With an alignment trick: each per-thread memory region is a fixed 8 pages, and regions are allocated at addresses that are multiples of that size. The handler is running on its `sigaltstack`, which lives inside its own region, so rounding the current stack pointer down to a 32 KiB boundary yields the region base, and the `sysmsg` struct sits at a known offset in the region's last page. The entire trick is four lines of pointer arithmetic in [`sysmsg_sp()` in `sysmsg.h`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg.h#L158-L170), dividing the address of a local variable by [`PER_THREAD_MEM_SIZE`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg.h#L136). The stack pointer _is_ the thread identity.

With its bearings established, the handler ([`__export_sighandler` in `sighandler_amd64.c`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L238)) performs the capture we sketched earlier, and the completeness is the point: the full general-purpose register file copied out of the `ucontext` (the snapshot of the interrupted thread's saved registers that the kernel hands every signal handler) into `ctx->ptregs` ([`gregs_to_ptregs`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L74-L104), a field-by-field translation from signal-frame layout to `struct user_regs_struct` layout), the entire FPU/XSAVE area (a [straight `memcpy` of `fp_len` bytes](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L303-L306), which on a machine with AVX-512 can be multiple kilobytes), the FS and GS base registers, the signal number, and the complete `siginfo_t`. This is a trap frame in every sense a kernel would recognize, including the [`orig_rax`/`-ENOSYS` convention](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L376-L382) described in an earlier footnote,[^origrax] which the handler reproduces byte-for-byte so the sentry's syscall machinery can consume `ptregs` exactly as Linux would consume its own entry frame. (Those same lines contain a bonus x32 detail: if `si_arch` says the syscall used the x32 ABI (a little-used Linux ABI that runs x86-64 CPUs with 32-bit pointers), which gVisor doesn't support, the handler nudges the syscall number by `0x86000000` so the sentry's dispatch will cleanly return `ENOSYS`.)

Then comes the handoff, [`switch_context()` in `sysmsg_lib.c`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L382-L404), and here the real design departs from my simplified four-step description in an important way. The handler publishes the context: it atomically sets `ctx->state` to `CONTEXT_STATE_SYSCALL` and [wakes the sentry with `futex(FUTEX_WAKE)` only if the sentry isn't already polling](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L393-L398). But it does **not** then sit and wait for _that context's_ reply. Instead it calls [`get_context()`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L318) to ask the shared context queue for the _next runnable context_, whosever it is. If some other guest task's syscall just completed, this thread picks up that task's saved state and resumes running it. The reply to the syscall we just submitted may well be picked up later by a different thread. Keep this in mind for the "contexts vs threads" section below; it is the mechanism that turns stub threads into interchangeable CPUs.

The restore path is the capture path in reverse, with one elegant shortcut: rather than copying the (potentially multi-kilobyte) FPU state back into the kernel-built signal frame, the handler simply [points the frame's FPU pointer (`ucontext->uc_mcontext.fpregs`) at `ctx->fpstate` in shared memory](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L415-L421). `rt_sigreturn` loads FPU state from wherever that pointer says. The general-purpose registers are written back into the frame, the FS/GS bases are restored if they changed, and the handler returns. The kernel then reconstitutes a CPU state that was chosen, field by field, by the sentry.

## SIGSEGV: page faults take the same trap vector

A hardware kernel receives page faults[^pagefault] through the same trap mechanism as syscalls, and so does systrap: a page fault in guest code is a synchronous `SIGSEGV` (or `SIGBUS`), and it lands in the very same handler. The payload differs: instead of syscall arguments, the handler forwards the `siginfo_t` (which carries the faulting address in `si_addr`) along with [the page-fault error code pulled from the signal frame](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L384-L387) (`REG_ERR`, captured only when the trap number says this was a real page fault), which encodes whether the access was a read, write, or instruction fetch and whether the page was present. It posts the context with `CONTEXT_STATE_FAULT` and switches away, exactly as for a syscall.

On the sentry side, [the task's run loop recognizes the fault](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/kernel/task_run.go#L311) and calls into the memory manager: [`mm.HandleUserFault`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/mm/syscalls.go#L38). This is real demand paging, implemented in Go. The mm looks up the vma covering the address; a vma, or virtual memory area, is a kernel's bookkeeping record for one contiguous region of address space (its bounds, its permissions, what backs it), and gVisor's mm keeps its own vmas for the guest. Does this task even have a valid mapping there, with the right permissions? If so, it ensures backing memory exists (allocating MemoryFile pages, reading from a file for a file-backed mapping, breaking copy-on-write if this was a write to a shared page: the trick by which many users share one physical page until somebody writes, at which point the writer gets a private copy), and then installs the host mapping into the stub's address space via the puppet syscall thread. The context is requeued, a stub thread resumes it, the faulting instruction re-executes, and this time the hardware page walk succeeds. The guest was never told any of this happened, which is exactly the contract a real kernel provides.

Two consequences of this design are easy to miss. First, _every_ application mapping in the stub is `MAP_SHARED` against the MemoryFile, even memory the app requested as `MAP_PRIVATE | MAP_ANONYMOUS`. The privacy and copy-on-write semantics the app observes are implemented entirely by the sentry's mm, not by the host kernel. Second, if the vma lookup fails (the app dereferenced a genuinely bad pointer), the host-level SIGSEGV does not propagate to the app. The sentry notes that the fault is unserviceable and delivers a _virtual_ SIGSEGV through its own implementation of Linux signal handling: the app's registered handler, its signal mask, its sigaltstack. The host signal and the guest signal are completely decoupled; one is the trap vector, the other is kernel-to-app communication.

## Contexts vs. threads

Linux schedules tasks onto CPUs; the task's register state is not welded to any particular core. Systrap reproduces this split exactly. A guest task's entire CPU state lives in its `thread_context` (the struct is the task), while sysmsg threads are anonymous, interchangeable executors. A subprocess supports up to [4,095 contexts](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L117-L121) but only creates about as many sysmsg threads as the sentry has usable cores (the cap is [derived from `GOMAXPROCS`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L108-L111), the number of OS threads the sentry's Go runtime will run at once). The ratio of contexts to threads is the ratio of tasks to CPUs.

The scheduler's run queue is the **context queue**: a lockless ring buffer in shared memory, which is to say a fixed-size circular queue in which producers and consumers coordinate through atomic operations rather than locks ([`contextQueue` in `context_queue.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/context_queue.go#L36-L56), mirrored byte-for-byte by [a C struct in `sysmsg_lib.c`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L45-L66); the Go file's comments on cache-line padding[^cacheline] are worth a read on their own). When the sentry finishes servicing a context (the syscall is done, the fault is resolved), it pushes the context's ID onto the ring. Stub threads pop from it: recall that `switch_context()` ends with the thread asking for the next runnable context, not the one it just gave up. A task really does trap on one thread and resume on another; the `thread_context` tracks which thread is currently carrying it ([`thread_id`, reset to an invalid sentinel on every switch](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L386-L392)) precisely because the answer keeps changing. When the sentry needs to preempt a running context (a timer expired, another task sent it a signal), it reads that field and [directs a `tgkill` interrupt at whichever thread the context happens to be riding](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L127-L158) at that moment (`tgkill` is plain `kill` aimed at one specific thread rather than a whole process).

On the sentry side the picture is pleasingly symmetric: each guest task is a goroutine,[^goroutine] and each goroutine owns a `sharedContext` wrapper around its `thread_context` slot ([`shared_context.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go)). While its task runs in the stub, the goroutine sleeps on the context's state word ([`sleepOnState`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L267-L282), whose [escalation ladder of timeouts](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L249-L255) of 10ms preempt, 5s checkup, and 30s stuck doubles as a watchdog: a context that stays stuck gets its whole subprocess killed). So the full picture of one guest task is: a `thread_context` struct holding its CPU state, a goroutine on the sentry side acting as its kernel half, and, only while it is actually executing, a temporary lease on some stub thread. The Go scheduler multiplexes the kernel halves; the context queue multiplexes the user halves.

## The fastpath

Everything described so far has a hidden tax: the futex. A `FUTEX_WAIT`/`FUTEX_WAKE` round trip means two host syscalls and a pair of context switches, which puts a floor of several microseconds under every intercepted syscall, which is painful when the syscall itself is something like `getpid`. Systrap's answer is to notice that both sides of the handoff are just watching a word in shared memory change, and that watching can be done without the kernel's help: spin on it.

Both sides have a fastpath, and they are independently switchable. On the **stub side**, idle sysmsg threads poll the context queue instead of sleeping in `FUTEX_WAIT`. Unbounded spinning would burn cores, so spinners register themselves in a shared [*spinning queue*](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L112-L138) ordered by how long each has been spinning; when the queue is contended or a spinner exceeds the deep-sleep timeout, the longest-spinning thread is evicted into a real futex sleep. The timeout is 20µs, and it must elapse twice in a row before the thread commits to sleep, a hedge against the imprecision of reading the TSC, the time-stamp counter: a register the CPU increments continuously, making it the cheapest clock available ([the comment block defining `deepSleepTimeoutNS`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L366-L381) documents the reasoning: a 20µs deep-sleep threshold matches the ~40µs round trip you'd pay in the pure sleeping case anyway).

On the **sentry side**, the insight is that you don't need N goroutines each futex-waiting on their own context's state word; you need _one_ poller watching all of them. That poller is the dispatcher ([`fastPathDispatcher.loop`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L420)), a role passed around among the waiting goroutines: whoever holds it walks the list of parked contexts, reads each state word, and when one flips, hands the dispatcher role to the next waiter and goes to process its own event. The dispatcher also enforces a [500ns *handshake timeout*](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L378-L380): if it queued a context and no stub thread has acknowledged it within that window, it kicks a sleeping stub thread awake. And the stub cooperates with the sentry's fastpath from its side: `switch_context()` checks a `sentry_fast_path` flag in the context and [_skips the `FUTEX_WAKE` syscall entirely_](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sysmsg_lib.c#L393-L398) when the sentry is polling. In full fastpath operation, the syscall handoff involves zero host syscalls in either direction: two processes communicating purely through cache-coherency traffic, the hardware machinery that carries one core's writes into another core's cache.[^cacheline]

Spinning is only a win when traffic is high enough to amortize the burned CPU, so none of this is statically configured. The sentry continuously measures handoff latencies (in TSC ticks, bucketed in [`metrics.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/metrics.go)) and a dedicated loop, [`controlFastPath()`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/metrics.go#L359), flips each fastpath on or off independently based on whether it is actually paying for itself.

## usertrap: skipping the signal too

The fastpath removed the futexes from the handoff, but the trap itself still costs a signal: kernel entry, filter evaluation, signal-frame construction, handler dispatch, and an `rt_sigreturn` on the way out. For hot syscall sites, systrap removes that too, by **binary-patching the guest**.

The opportunity comes from an x86 coincidence of sizes, spelled out in the constants at the top of [`usertrap_amd64.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L36-L68). The overwhelmingly common syscall sequence emitted by compilers and libc is `mov $sysno, %eax; syscall`: a 5-byte immediate move (an immediate is a constant baked directly into the instruction's bytes) plus the 2-byte syscall instruction, 7 bytes total. An absolute indirect jump, `jmp *addr32` (`ff 24 25` + a 4-byte address), is _also_ exactly 7 bytes. So when the SIGSYS handler fires, it [inspects the bytes before the saved program counter](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L317-L374), and if they match the pattern (and the immediate agrees with both `si_syscall` and `rax`, since the bytes might otherwise be the tail of some longer instruction; the source carries [a FIXME admitting x86 can't be safely decoded in reverse](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L327-L330)), it reports the context state as `CONTEXT_STATE_SYSCALL_NEED_TRAP` instead of plain `SYSCALL`. The sentry then overwrites the 7-byte sequence ([`PatchSyscall`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L244)) with a jump into a **trap table** it has mapped into the app's address space: [256 slots of 80 bytes each](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L36-L52). Each table entry is a small trampoline (a short run of code you jump into, which sets things up and immediately jumps onward) that records the return address and syscall number, then enters `__syshandler`, stub code that feeds the same `switch_context()` machinery as the signal handler, locating its `sysmsg` struct through the GS base register. From then on, executing that call site traps into the "kernel" with a plain jump: no SIGSYS, no signal frame, no host kernel involvement whatsoever. (This is also why GS matters: if the app itself claims GS via `arch_prctl(ARCH_SET_GS)`, [patching is disabled for that context](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L192-L195) and existing patches are backed out.)

Patching live code that other threads may be executing _right now_ requires a protocol, and the one in [`PatchSyscall`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L296-L343) is a three-step gem. **Step one** writes `0x06` (a byte that decodes to no valid x86-64 instruction, so any thread that tries to execute it faults) over the first byte of the `syscall` instruction (offset 5 of the sequence, not offset 0: the 5-byte `mov` ahead of it still decodes fine). Any thread racing through the site now executes the mov, faults on the sentinel, and gets restarted: the fault travels the normal trap path to the sentry, which recognizes an in-progress patch and [rewinds the program counter](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L697) to re-execute the site; a thread whose syscall already trapped concurrently does the same check itself and [rewinds by 7 when it sees the sentinel](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L360-L369). **Step two** overwrites bytes 1–6 with the jump's bytes 1–6. This _sounds_ like it should break step one's invariant (those bytes include offset 5!), but here is my favorite single design decision in all of usertrap: the trap table is [deliberately mapped in the range `0x60000`–`0x70000`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L190-L196) precisely so that the third byte of every trap address, as laid out in memory, is itself `0x06` (x86 is little-endian: multi-byte values are stored least-significant byte first, so an address like `0x60f40` is encoded as `40 0f 06 00`). Mid-patch, offset 5 keeps holding an invalid opcode no matter which writes have landed ([the comment explaining this](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/usertrap/usertrap_amd64.go#L297-L320) is worth reading twice). **Step three** writes the `jmp` opcode over byte 0, last, atomically flipping every future reader to the completed jump. Even the handler's own inspection of the bytes before the PC is hardened: that read can itself fault (the preceding bytes may sit on an unmapped page), so the handler arms a [`fault_jump` escape hatch](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c#L273-L279) that turns a nested SIGSEGV into "skip the load and move on" rather than a recursive trap.

## Trust boundary

It is worth stepping back and asking an uncomfortable question about everything above: the sysmsg handler, the context queue, and the `thread_context` structs all live inside the stub process, writable by the very code we are sandboxing. A malicious app doesn't need an exploit to scribble over `thread_context`; it's mapped read-write in its own address space. Hostile graffiti on the shared state is not a vulnerability in this design; it's an assumed input.

The sentry therefore treats shared memory the way a kernel treats user memory: as untrusted data to be read carefully and validated. The [comment at the top of `shared_context.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L36-L45) is explicit: all access is atomic "because there is no (trusted) synchronization between the sentry and the stub processes," and "data read from shared memory may require validation before it can be used." You can watch the discipline in the code: the thread ID is [loaded exactly once before use, with a comment calling out the TOCTOU risk](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L131-L137) of reading it twice (TOCTOU, time-of-check-to-time-of-use: a value that is validated and then read again can change between the two reads); timestamps written by the stub are sanity-checked and logged as ["likely memory tampering detected"](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L217-L242) when they're impossible; a context reporting an inconsistent state gets its [whole subprocess killed](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/shared_context.go#L267-L282) rather than reasoned with.

So what does an attacker actually get by corrupting the shared state? Remarkably little, and this is the measure of the design. The register file in `thread_context` is nothing more than a syscall request: the app could produce any register state it wants by simply executing instructions, so forging one grants no new capability. Pointers in that state are validated against the task's own memory map by the sentry's mm, same as any user pointer. And the host kernel remains out of reach regardless: the seccomp filter is irrevocable, its allowlist is a handful of syscalls with argument and instruction-pointer constraints, and the guest can't even `mprotect` the stub's code pages, because `mprotect` is a syscall and syscalls trap. Corrupting the stub lets the app break _itself_. The actual security boundary is the sentry: a memory-safe Go program that exposes only its syscall ABI to the guest, runs under its own, separate seccomp sandbox, and delegates host filesystem access to yet another process, the gofer, so that even a fully compromised sentry holds no direct rights to the host's files. The stub, signal handler and all, sits on the untrusted side of that line: gVisor does not even trust its own trap vector.
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

- a shared **mailbox page** with a futex-backed state machine (`APP → EVENT → REPLY → APP`), the toy `switch_context`;
- a **memfd** as the sentry's MemoryFile, mapped fully by the sentry;
- the app's heap: a `PROT_NONE` reservation (address space claimed, but with all access forbidden, so the first touch of any page faults). Nothing is mapped until the app faults and the sentry provides backing memory;
- a **SIGSYS handler** (seccomp trap → ship syscall to sentry → write back the return value);
- a **SIGSEGV handler** (ship fault address → `mmap` the page the sentry chose → re-execute).

The demo app exercises all of it, including two deliberate lessons:

1. `getpid()` returns 1: the sentry *is* the kernel, and it virtualizes whatever it likes.
2. `write()` from a stack buffer fails with `EFAULT`: the sentry refuses memory it doesn't manage, which is the toy version of why gVisor backs every application page with its own memfd.

### The code

The code is one compilable file, [`toy_systrap.c`](https://github.com/LandonTClipp/LandonTClipp.github.io/blob/main/code/gvisor-systrap-toy/toy_systrap.c), presented here one component at a time; concatenate the blocks top to bottom (with a blank line between each) and you have the file, byte for byte. Linux only, x86_64 or arm64.

#### Preliminaries

First, the boilerplate. The only interesting part is the per-architecture macros: the real sysmsg handler has `gregs_to_ptregs` to translate the entire signal frame, while the toy only needs to know where the return value and the first three arguments live.

```c title="toy_systrap.c, part 1: preliminaries"
// toy_systrap.c: a miniature gVisor systrap in ~300 lines.
//
// Two processes after a fork(): a "sentry" (the userspace kernel) and an
// "app" (the sandboxed workload). The app's write() and getpid() syscalls
// never reach the host kernel, and its heap is demand-paged by the sentry.
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

// Per-architecture access to the registers saved in the signal frame.
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
```

#### The shared mailbox

The shared mailbox collapses three pieces of the real design into one struct: `thread_context` (the event payload), the context queue (whose turn it is), and the futex handoff. `post_event_and_wait()` is the toy `switch_context()`, minus the part where it picks up a _different_ context to run, because with one app there is only one context. The ownership protocol is the `state` word: `APP → EVENT → REPLY → APP`.

```c title="toy_systrap.c, part 2: the shared mailbox"
// ======================= The shared mailbox ================================

enum event { EVENT_SYSCALL = 1, EVENT_FAULT, EVENT_EXIT };

enum state {          // who owns the mailbox
  STATE_APP = 0,      // app is running, mailbox idle
  STATE_EVENT = 1,    // app posted an event, sentry's turn
  STATE_REPLY = 2,    // sentry posted a reply, app's turn
};

struct thread_context {
  _Atomic uint32_t state;  // the futex word
  int event;               // EVENT_*
  int sysno;               // EVENT_SYSCALL: syscall number...
  uint64_t args[3];        //   ...and its first three arguments
  uint64_t fault_addr;     // EVENT_FAULT: the faulting address
  int64_t retval;          // the reply: return value, or a memfd offset
};

// Globals are set up before fork(), so both processes inherit them.
#define HEAP_PAGES 4
static struct thread_context *ctx;  // MAP_SHARED: the mailbox page
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

// Hand the mailbox to the sentry; sleep until it hands back a reply.
static void post_event_and_wait(void) {
  atomic_store(&ctx->state, STATE_EVENT);
  futex_wake(&ctx->state);
  uint32_t s;
  while ((s = atomic_load(&ctx->state)) != STATE_REPLY)
    futex_wait(&ctx->state, s);
  atomic_store(&ctx->state, STATE_APP);
}
```

#### The seccomp filter

The seccomp filter, inverted relative to the real thing: real systrap default-traps everything and allowlists its own handler code by instruction pointer; the toy traps exactly two syscalls and allows the rest. The toy's handler needs `futex`, `mmap`, and `rt_sigreturn` exactly as the real sysmsg code does, but instead of instruction-pointer checks, it simply never traps them in the first place.

```c title="toy_systrap.c, part 3: the seccomp filter"
// ======================= The seccomp filter ================================

// Trap write() and getpid(); allow everything else.
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
```

#### The SIGSYS handler

The toy `__export_sighandler`, syscall half. Note what seccomp hands us for free: the syscall number in `si_syscall`, and a saved program counter that already points past the `syscall` instruction. The handler ships three registers instead of a full trap frame, waits for the verdict, writes one register back, and returns.

```c title="toy_systrap.c, part 4: the SIGSYS handler"
// ======================= The SIGSYS handler ================================

// Every trapped syscall lands here, courtesy of seccomp.
static void sigsys_handler(int signo, siginfo_t *si, void *ucv) {
  ucontext_t *uc = ucv;
  ctx->event = EVENT_SYSCALL;
  ctx->sysno = si->si_syscall;  // seccomp fills this in
  ctx->args[0] = REG_A0(uc);
  ctx->args[1] = REG_A1(uc);
  ctx->args[2] = REG_A2(uc);
  post_event_and_wait();
  REG_RET(uc) = ctx->retval;
  // For a seccomp trap, the saved PC already points after the syscall
  // instruction: returning resumes the app as if the kernel had returned.
}
```

#### The SIGSEGV handler

The fault half. One deliberate difference from real systrap: here the faulting process installs its own mapping with `mmap(MAP_FIXED)`, whereas in real gVisor the sentry installs it remotely through the puppet syscall thread. The effect is identical: returning from the handler re-executes the faulting instruction, which now succeeds.

```c title="toy_systrap.c, part 5: the SIGSEGV handler"
// ======================= The SIGSEGV handler ===============================

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
  post_event_and_wait();
  // The sentry chose backing memory: ctx->retval is a memfd offset.
  // Install the mapping, then return to re-execute the faulting
  // instruction, which now succeeds.
  uint64_t page = addr & ~(uint64_t)(pagesz - 1);
  mmap((void *)page, pagesz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
       memfd, ctx->retval);
}

static void install_handlers(void) {
  struct sigaction sa = {.sa_flags = SA_SIGINFO};
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sigsys_handler;
  sigaction(SIGSYS, &sa, NULL);
  sa.sa_sigaction = sigsegv_handler;
  sigaction(SIGSEGV, &sa, NULL);
}
```

#### The app

The workload. It looks like a perfectly normal program (`strcpy`, `write`, `getpid`), but notice there is no `printf` after the filter goes in: stdio buffers live in private memory the sentry refuses to read, so the app can only `write()` out of its managed heap, down to hand-rolling its own `itoa`. That constraint _is_ lesson two.

```c title="toy_systrap.c, part 6: the app"
// ======================= The app ===========================================

static int fmt_num(char *dst, long v) {  // minimal itoa, v >= 0
  char tmp[20];
  int n = 0;
  do {
    tmp[n++] = '0' + (v % 10);
    v /= 10;
  } while (v > 0);
  for (int i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
  return n;
}

static void app_main(void) {
  // 1. Touch unbacked memory. The strcpy faults on page 0 of the heap;
  //    the sentry assigns backing memory and we resume mid-instruction.
  char *msg = (char *)app_heap;
  strcpy(msg, "hello from the sandbox\n");

  // 2. A syscall: seccomp -> SIGSYS -> the sentry performs the write.
  write(1, msg, strlen(msg));

  // 3. The sentry virtualizes getpid(): every sandboxed app is pid 1.
  long pid = getpid();
  char *msg2 = (char *)app_heap + pagesz;  // page 1: faults again
  const char *s = "the sentry says my pid is ";
  memcpy(msg2, s, strlen(s));
  int n = strlen(s);
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
```

#### The sentry

The entire kernel: two `case` statements. The `write` path is the toy `copy_from_user`: after checking that the pointer lies in memory it manages, the sentry reads the app's buffer through `sentry_view`, its own mapping of the memfd. The run loop is pure slow path: futex wait, handle the event, futex wake. No spinning, no dispatcher, no fastpath.

```c title="toy_systrap.c, part 7: the sentry"
// ======================= The sentry ========================================

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
      // Read the app's buffer through the sentry's own memfd mapping.
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
                        "%ld\n", ctx->fault_addr, off / pagesz);
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
```

#### Setup: one process becomes two

Finally, the setup. Everything shared is created _before_ the `fork()` so that both processes inherit the same addresses: the mailbox page, the memfd and the sentry's view of it, and the app heap: a `PROT_NONE` reservation, the toy's vma, with nothing mapped behind it until the sentry says so.

```c title="toy_systrap.c, part 8: setup"
// ======================= Setup: one process becomes two ====================

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

On a Mac, in a Linux container (Apple Silicon: use a native arm64 container; Rosetta-emulated amd64 containers don't emulate the seccomp/futex dance faithfully and the toy will hang):

```
$ docker run --rm -v "$PWD":/w -w /w gcc:14 \
    bash -c 'gcc -O2 -Wall -o toy_systrap toy_systrap.c && ./toy_systrap'
```

Output (sentry log on stderr, app output on stdout, so your terminal may interleave them differently):

```
[app]    starting; my real pid is 11
[sentry] fault at 0xffff8f5ed000 -> backing with memfd page 0
[sentry] write(1, app:0xffff8f5ed000, 23) -> doing it myself
hello from the sandbox
[sentry] getpid() -> telling the app it is pid 1
[sentry] fault at 0xffff8f5ee000 -> backing with memfd page 1
[sentry] write(1, app:0xffff8f5ee000, 28) -> doing it myself
the sentry says my pid is 1
[sentry] write(1, 0xfffffdf365c8, 25) -> EFAULT (not app memory that I manage)
[sentry] write(1, app:0xffff8f5ee000, 45) -> doing it myself
write from private memory failed with EFAULT
[sentry] app exited; shutting down
```

Read that output against the code once and the whole trick clicks: the app called `strcpy`, `write`, and `getpid` like any program, but its memory was allocated by another process's page-fault service, its output was performed by another process, and its pid is a lie.

## What the real thing adds

The toy is the skeleton. Real systrap (in [`pkg/sentry/platform/systrap/`](https://github.com/google/gvisor/tree/release-20261116.0/pkg/sentry/platform/systrap)) hardens and generalizes every piece:

- **Everything is trapped.** The filter default-traps all syscalls and allowlists only what the handler itself needs; the handler captures the full register file and FPU state (see `__export_sighandler` in [`sysmsg/sighandler_amd64.c`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg/sighandler_amd64.c)), on a dedicated `sigaltstack`, with care for races the toy ignores.
- **Contexts are scheduled.** `switch_context` doesn't just reply to one app; it parks the stub thread and hands it whichever ready context the sentry's scheduler picks, so a pool of stub threads multiplexes many tasks.
- **The kernel is skipped entirely when possible.** The SIGSYS path inspects the syscall site and, if it's the common `mov $sysno, %eax; syscall` pattern, binary-patches it into a jump to `__syshandler`, after which that call site traps into the handler *without a signal, without entering the host kernel at all* (`pkg/sentry/usertrap`).
- **The memory manager is real.** The sentry side of the fault path is [`mm.HandleUserFault`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/mm/syscalls.go): vmas, pmas, copy-on-write, demand paging from files, all dispatched from the task run loop in [`kernel/task_run.go`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/kernel/task_run.go).
- **And the sentry is an entire kernel** (syscalls, a VFS, a network stack) behind that one entry point.

The part I find elegant: there's no magic layer anywhere. The "kernel entry" is `sigaction`, the "context switch" is a futex, the "page tables" are `mmap(MAP_FIXED)` over a memfd, and the application can't tell the difference.

[^origrax]: The arguments to syscalls are stored in CPU registers. So `thread_context` is populated with the values of those registers. In x86, those are `rax` (the syscall number) with arguments stored in `rdi`, `rsi`, `rdx`, `r10`, `r8`, and `r9`. On arm64, `x8` stores the syscall number and arguments are in `x0-x5`. Another interesting thing that the handler does, right before handoff, is this:
    
    ```c
    ctx->ptregs.orig_rax = ctx->ptregs.rax;
    ctx->ptregs.rax = (unsigned long)-ENOSYS;
    ```

    This is a precise imitation of what Linux itself does at syscall entry: the kernel stashes the syscall number in `orig_rax` so that it survives, and pre-loads `rax` with `-ENOSYS` (so an unhandled syscall "returns" `ENOSYS` by default, and signal/restart logic can distinguish the number from the return value). The sysmsg handler reproduces the kernel's own trap-frame conventions so the sentry's syscall machinery can consume `ptregs` exactly as if it were a kernel reading its own entry frame. This is a beautiful example of how the sentry attempts to deeply mimic how Linux handles syscall conventions.

[^futex]: A futex ("fast userspace mutex") is the kernel's building block for locks and condition variables. `futex(FUTEX_WAIT, addr, val)` puts the calling thread to sleep, but only if the 32-bit word at `addr` still contains `val`; `futex(FUTEX_WAKE, addr, n)` wakes up to `n` threads sleeping on that word. The word itself lives in ordinary user memory, so the uncontended path (nobody to wake, nothing to wait for) costs no syscall at all. How futexes work _across processes_ is even more interesting. When the sentry's task goroutine parks (`sleepOnState`), an OS thread of the sentry process is literally sitting in `futex(FUTEX_WAIT, &ctx->state, old_val)`. The kernel computes a futex key for that address. For a process-private futex the key would be `(this process's mm, virtual address)`, which is useless across processes. But this futex is on a shared mapping (no FUTEX_PRIVATE_FLAG), so the kernel resolves the virtual address through the page tables to the backing object and keys it as `(inode of the memfd, page offset within the file, offset within the page)`. It then hangs the sleeping thread on the hash bucket for that key. 

    The wake side recomputes the same key from a different address. When the sysmsg handler calls `futex(FUTEX_WAKE, &ctx->state, 1)`, the address it passes is a different virtual address in a different process. The kernel does the same resolution: walk the stub's page tables, find the backing page. Because the `thread_context` region is one memfd mapped into both processes, it arrives at the same (inode, offset) key. It looks that key up in the hash table, finds the sentry thread enqueued under it, and wakes it. The kernel never knows these two processes are "sentry" and "stub," or that they're related at all; it just matched two addresses that resolve to the same page of the same file.

[^rostack]: There's a subtle ordering concern inside this initialization dance that shows how paranoid the code is about its own threads. Calls like `sigaltstack` and `sigaction` take _pointers_ to in-memory argument structs, and those structs live in stub memory, where another, already-running stub thread could theoretically race in and modify them between the moment the sentry writes them and the moment the kernel reads them. So the sentry first maps the new thread's shared stack **read-only**, executes the pointer-taking syscalls remotely while the arguments are immutable, and only then remaps the stack read-write for normal operation (see [the comment and `mapStack(addr, true)` call in `createSysmsgThreadLocked`](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/subprocess.go#L1215-L1232); `mapStack`'s `readOnly` parameter is [here](https://github.com/google/gvisor/blob/release-20261116.0/pkg/sentry/platform/systrap/sysmsg_thread.go#L60)). The trust boundary section at the end of this post explains why this level of suspicion toward the stub's own memory is warranted.

[^seccomp]: seccomp ("secure computing mode") is a Linux facility that lets a process attach a syscall filter to itself. From then on, the kernel runs the filter at the entry of every syscall the process makes, before executing the syscall itself. The filter is a small program in BPF (Berkeley Packet Filter) bytecode, a deliberately restricted instruction set that the kernel can run safely because it can verify the program terminates. The filter may inspect the syscall number, its arguments, and the address of the calling instruction, and must return a verdict: allow the syscall, fail it with an error, kill the process, or, the one systrap is built on, `SECCOMP_RET_TRAP`: skip the syscall and deliver a signal instead. Its day job is sandboxing; Docker and Chrome both use it to shrink the kernel's attack surface.

[^signals]: A signal is the kernel's mechanism for interrupting a process to tell it something happened: `SIGINT` when you press Ctrl-C, `SIGSEGV` on a bad memory access, and so on. A process may register a function, a signal handler, for a given signal. To deliver it, the kernel pauses the thread wherever it happens to be, saves its complete CPU state onto the stack (the "signal frame"), and redirects execution into the handler; when the handler returns, the kernel restores the saved state and the thread continues as if nothing happened. Most signals are asynchronous and can arrive at any moment. `SIGSYS` ("bad system call") is one of the synchronous ones: it is caused by the thread's own current instruction and delivered on the spot, before the thread executes anything else, which is exactly the property a trap vector needs.

[^mmap]: `mmap` asks the kernel to wire a range of a file into the process's address space, so that ordinary loads and stores at those addresses read and write the file's pages directly, with no `read`/`write` calls involved. `MAP_SHARED` makes the mapping a true window rather than a private copy: every process that maps the same file range sees every write. `MAP_FIXED` says "place the mapping at exactly this virtual address," replacing whatever was there, which is what lets the sentry put pages at the addresses the app expects to find them.

[^ptrace]: `ptrace` is Linux's debugger interface, the machinery underneath `gdb` and `strace`. A tracer process can stop a tracee, read and write its registers and its memory, single-step it, and get notified when it receives a signal or enters a syscall. It is powerful but expensive: every peek and poke is a syscall of its own, and every stop costs context switches. gVisor's older ptrace platform drove every guest syscall through this interface; systrap demotes it to a setup-time tool.

[^xsave]: "FPU state" is shorthand for every register that is not general-purpose: the floating-point registers and the SSE/AVX vector registers chief among them. x86's XSAVE family of instructions dumps all of it into memory as one architecturally defined blob whose size depends on which features the CPU supports. A machine with AVX-512 has thirty-two 512-bit vector registers (2 KiB of state by themselves), which is why the blob can run to several kilobytes.

[^tls]: On x86-64, FS and GS are two leftover segment registers repurposed as free-floating base pointers: an instruction can address memory relative to whatever address they hold. By convention, the C runtime points FS at a per-thread block of memory, and "thread-local" variables compile down to FS-relative loads, so the FS base effectively _is_ a thread's identity as far as userspace code is concerned. The guest application owns FS for exactly this purpose. GS is ordinarily unused by Linux userspace, which is why usertrap, later in this post, feels free to claim it.

[^pagefault]: When code touches a virtual address, the CPU translates it to physical memory through the page tables. If no valid translation exists, or the access violates the page's permissions, the CPU raises a page fault: a hardware trap into the kernel. Faults are not inherently errors. Kernels deliberately leave pages unmapped so that the first touch traps, letting memory be allocated lazily on first use (demand paging). Only when the kernel decides the address is genuinely invalid does the fault surface as the `SIGSEGV` that kills your program.

[^goroutine]: Goroutines are Go's lightweight threads: the Go runtime multiplexes thousands of them onto a small pool of OS threads, parking and resuming them in userspace for roughly the price of a function call. `GOMAXPROCS` caps how many OS threads execute Go code at once and defaults to the machine's CPU count. This is what makes "one goroutine per guest task" affordable: a parked goroutine costs kilobytes, not an OS thread.

[^cacheline]: CPU cores do not read RAM a byte at a time; memory moves through the cache hierarchy in fixed chunks called cache lines, 64 bytes on x86. The coherency protocol keeps all cores agreeing on a line's contents, so when one core writes to a line, every other core's cached copy is invalidated and must be re-fetched. Two consequences matter here. First, unrelated variables sharing a line make writes to one slow down readers of the other ("false sharing"), which the context queue's padding avoids. Second, a polling loop reading a shared word costs nothing until the writer actually writes, at which point the update propagates from cache to cache in tens of nanoseconds, faster than any syscall could.
