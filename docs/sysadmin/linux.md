---
title: Linux
---

sysfs
------

https://en.wikipedia.org/wiki/Procfs
https://docs.kernel.org/filesystems/proc.html

sysfs is a special filesystem maintained by the linux kernel that allows you to inspect the state of running processes.

```
$ ls -lah /proc/21/
total 0
dr-xr-xr-x   9 root root 0 Oct 28 17:16 .
dr-xr-xr-x 238 root root 0 Oct 28 17:16 ..
dr-xr-xr-x   2 root root 0 Dec  8 15:43 attr
-rw-r--r--   1 root root 0 Dec  8 15:43 autogroup
-r--------   1 root root 0 Dec  8 15:43 auxv
-r--r--r--   1 root root 0 Dec  8 15:43 cgroup
--w-------   1 root root 0 Dec  8 15:43 clear_refs
```

There are a lot of useful bits here. For example, you can inspect all open file descriptors for a process. In fact, this is what `lsof` uses to show open files:

```
$ ls -l /proc/79808/fd                    
lr-x------ 1 ltclipp ltclipp 64 Dec  8 15:44 0 -> /dev/null  
lrwx------ 1 ltclipp ltclipp 64 Dec  8 15:44 1 -> 'socket:[126462260]'
lrwx------ 1 ltclipp ltclipp 64 Dec  8 15:44 10 -> /tmp/foo.txt
```

You can view your own kernel info:

```
$ ls -l /proc/self/
dr-xr-xr-x   2 ltclipp ltclipp   0 Dec  8 15:47 attr
-rw-r--r--   1 ltclipp ltclipp   0 Dec  8 15:47 autogroup
-r--------   1 ltclipp ltclipp   0 Dec  8 15:47 auxv
-r--r--r--   1 ltclipp ltclipp   0 Dec  8 15:47 cgroup
--w-------   1 ltclipp ltclipp   0 Dec  8 15:47 clear_refs
-r--r--r--   1 ltclipp ltclipp   0 Dec  8 15:47 cmdline
-rw-r--r--   1 ltclipp ltclipp   0 Dec  8 15:47 comm
-rw-r--r--   1 ltclipp ltclipp   0 Dec  8 15:47 coredump_filter
```

strace
------

strace can be used to inspect the system calls being made by an application.

```
$ strace -f cat /tmp/hello.txt |& egrep 'hello'
execve("/bin/cat", ["cat", "/tmp/hello.txt"], 0x7fffffffc1a0 /* 115 vars */) = 0
openat(AT_FDCWD, "/tmp/hello.txt", O_RDONLY) = 3
read(3, "hello world\n", 131072)        = 12
write(1, "hello world\n", 12hello world
```

FUSE
-----

placeholder

NFS
---

placeholder

Kernel Modules
--------------

https://wiki.archlinux.org/title/Kernel_module#:~:text=Kernel%20modules%20are%20pieces%20of,as%20built%2Din%20or%20loadable.

The linux kernel allows you to hot-load pieces of code, usually drivers (or really anything that needs to run in kernel space).

You can view the list of available modules:

```
 $ kmod list
Module                  Size  Used by
nfsv3                  57344  0
nfs_acl                16384  1 nfsv3
nfs                   389120  1 nfsv3
lockd                 122880  2 nfsv3,nfs
grace                  16384  1 lockd
```

DRAC
-----

https://en.wikipedia.org/wiki/Dell_DRAC

A DRAC (Dell Remote Access Controller) is a hardware unit within a server chassis that is capable of monitoring, deploying, and interacting with the main server hardware and host outside of the typical kernel. It's often integrated into the motherboard itself, and acts as a standalone computer that you can log into and issue commands to. 

The main benefit of a DRAC is being able to independently execute commands to the host kernel (either through a console or through power cycling commands via hardware), monitoring the health of hardware components, configuring hardware, BIOS, host OS, and various other facets.

Kernel Parameters
------------------

### nohz_full

https://www.kernel.org/doc/Documentation/timers/NO_HZ.txt

An "adaptive-tick" CPU is one where the kernel can temporarily disable the scheduling clock ticks if there is only one runnable task on the core. This is useful in realtime or latency-sensitive applications that need to not be interrupted for scheduling work. The `nohz_full` kernel boot parameter specifies which cores should be the adaptive-tick cores.

```
nohz_full=4-7
```

Cores which are not currently configured (by the kernel's runtime logic) to receive scheduling interrupts are considered to be "dyntick-idle":

!!! quote

    An idle CPU that is not receiving scheduling-clock interrupts is said to be "dyntick-idle", "in dyntick-idle mode", "in nohz mode", or "running tickless".  The remainder of this document will use "dyntick-idle mode".

