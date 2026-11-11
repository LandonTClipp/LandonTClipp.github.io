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
