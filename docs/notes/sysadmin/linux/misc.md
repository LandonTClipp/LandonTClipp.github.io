---
title: Linux
icon: simple/linux
---

Filesystems
-----------

### FUSE

![FUSE diagram](https://upload.wikimedia.org/wikipedia/commons/0/08/FUSE_structure.svg)

FUSE stands for **Filesystem in Userspace**. It is a software interface that allows non-privileged applications to provide their own filesystem and mount it within the Linux file namespace. The FUSE module (which is a kernel module) provides a software bridge to the kernel interfaces.

### NFS

placeholder

### [VFS](https://www.kernel.org/doc/html/next/filesystems/vfs.html)

VFS, or Virtual File System, is a component of the Linux kernel that provides the filesystem interface to userspace programs. The VFS is what implements open, stat, chmod, and other similar filesystem-related system calls. The pathnames passed to these calls is used by the VFS to lookup the directory entry cache, aka dentry cache or dcache). This allows very fast lookups of dentries without needing to reference the backing filesystem.


### procfs

https://en.wikipedia.org/wiki/Procfs
https://docs.kernel.org/filesystems/proc.html

procfs is a special filesystem maintained by the linux kernel that allows you to inspect the state of running processes.

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

### sysfs

sysfs is a kernel-maintained filesystem for interacting with various kernel subsystems, hardware devices, and device drivers.

#### PCIe Devices

You can use sysfs to read the information about PCIe devices.

```
# ls -lah /sys/bus/pci/devices/0000:db:00.0/
total 0
drwxr-xr-x 4 root root    0 Jul 17  2023 .
drwxr-xr-x 6 root root    0 Jul 17  2023 ..
-r--r--r-- 1 root root 4.0K May 10 18:03 aer_dev_correctable
-r--r--r-- 1 root root 4.0K May 10 18:03 aer_dev_fatal
-r--r--r-- 1 root root 4.0K May 10 18:03 aer_dev_nonfatal
-r--r--r-- 1 root root 4.0K May 10 18:03 ari_enabled
-rw-r--r-- 1 root root 4.0K May 10 18:03 broken_parity_status
-r--r--r-- 1 root root 4.0K Jul 17  2023 class
-rw-r--r-- 1 root root 4.0K Jul 17  2023 config
-r--r--r-- 1 root root 4.0K May 10 18:03 consistent_dma_mask_bits
-r--r--r-- 1 root root 4.0K May 10 18:03 current_link_speed
-r--r--r-- 1 root root 4.0K May 10 18:03 current_link_width
-rw-r--r-- 1 root root 4.0K May 10 18:03 d3cold_allowed
```

The device ID can be discovered by using `lspci`.

##### `resource` files

The file located at `/sys/bus/pci/devices/*/resource` provides ASCII text that describes the host addresses of PCI resources for that device. For each region, there is a corresponding `/sys/bus/pci/devices/*/resource*` file that contains the contents of that region. You must memory-map to this file in order to access it.

For example, using `lspci` we can introspect the regions:

```
# lspci -n -s 0000:01:00.1 -vv
0000:01:00.1 0200: 8086:10c9 (rev 01)
        Subsystem: 10a9:8028
        Control: I/O+ Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx+
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- SERR- <PERR- INTx-
        Latency: 0, Cache Line Size: 64 bytes
        Interrupt: pin B routed to IRQ 40
        Region 0: Memory at b2140000 (32-bit, non-prefetchable) [size=128K]
        Region 1: Memory at b2120000 (32-bit, non-prefetchable) [size=128K]
        Region 2: I/O ports at 2000 [size=32]
        Region 3: Memory at b2240000 (32-bit, non-prefetchable) [size=16K]
```

These regions can be device memory, IO ports, or other resources. The exact contents of the memory is going to be specific to the device in question.

Readings:

- https://www.kernel.org/doc/Documentation/filesystems/sysfs-pci.txt
- https://techpubs.jurassic.nl/manuals/linux/developer/REACTLINUX_PG/sgi_html/ch07.html

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

Security
--------

### [SELinux](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/using_selinux/getting-started-with-selinux_using-selinux)

Security-Enhanced Linux is a linux security module that provides the ability to implement access control policies. In Linux, the default access control mechanisms are done through what's called Discretionary Access Controls (DAC). The granularity of a DAC is only based off of user, group, and "other" permissions, and are applied to specific files.

SELinux implements [Mandatory Access Control (MAC)](https://en.wikipedia.org/wiki/Mandatory_access_control). System resources have what's called an SELinux context. The context, otherwise known as an SELinux Label, abstracts away the underlying resources and instead focuses on only the security properties of the underlying object.


### sudoers

The `/etc/sudoers` file is a file on Linux systems that describes various actions that users are allowed to take as the root user. The man page describes in depth the details of this file:

```
SUDOERS(5)                                                                                                      File Formats Manual                                                                                                     SUDOERS(5)

NAME
       sudoers - default sudo security policy plugin

DESCRIPTION
       The sudoers policy plugin determines a user's sudo privileges.  It is the default sudo policy plugin.  The policy is driven by the /private/etc/sudoers file or, optionally, in LDAP.  The policy format is described in detail in the
       SUDOERS FILE FORMAT section.  For information on storing sudoers policy information in LDAP, see sudoers.ldap(5).
```

### ACL

An ACL, or Access Control List, commonly refers to extra access policies that are applied to specific files or directories. You can view ACLs in most POSIX-compatible filesystems using the `getfacl` command:

```
ubuntu@primary:/tmp$ echo hello > test.txt
ubuntu@primary:/tmp$ getfacl test.txt
# file: test.txt
# owner: ubuntu
# group: ubuntu
user::rw-
group::rw-
other::r--
```

You can apply ACLs using the `setfacl` command (some additional examples can be found [here](https://www.ibm.com/docs/en/zos/3.1.0?topic=scd-setfacl-set-remove-change-access-control-lists-acls#sfacl__title__5)):

```
ubuntu@primary:/tmp$ setfacl -m user:ubuntu:rw test.txt
ubuntu@primary:/tmp$ getfacl test.txt
# file: test.txt
# owner: ubuntu
# group: ubuntu
user::rw-
user:ubuntu:rw-
group::rw-
mask::rw-
other::r--
```

As you can see, this allows you finer grained control over access to your filesystem objects.

## [Kickstart](https://en.wikipedia.org/wiki/Kickstart_(Linux))

Kickstart is an installation mechanism provided by Redhat that allows you to install and configure operating systems in an automated fashion. Cobbler is used to automate the kickstart configuration process.

## POSIX Signals

| number | name | default action | Shortcut | description |
|--------|------|----------------|----------|-------------|
| 1 | SIGHUP | Terminate | | Hang up controlling terminal or process. Often used by many systems to mean "please re-read and reload config." |
| 2 | SIGINT | Terminate | ++ctrl+c++ | Interrupt from keyboard.|
| 3 | SIGQUIT | Core | ++ctrl+backslash++ | Quit from keyboard. Similar to SIGINT, but it dumps a core in addition to terminating the program. |
| 4 | SIGILL | Core | | Illegal instruction |
| 6 | SIGABRT | Core | | Abort signal from [abort(3)](https://www.man7.org/linux/man-pages/man3/abort.3.html) |
| 8 | SIGFPE | Core | | Floating point exception. |
| 9 | SIGKILL | Terminate | | Forcefully terminate a program. This signal is not catchable. |
| 15 | SIGTERM | Terminate | | Gracefully terminate a program. This is similar in behavior to SIGINT, but it cannot be sent from the keyboard. Parent processes will typically send this signal to its children upon termination. |
| 19, 18, 25 | SIGCONT | Continue | | Continue execution of a process that was stopped by SIGSTOP. You can also use the `bg` bash command to continue the process in the background. See [Backgrounding a Terminal Process](/programming/bash/#backgrounding-a-terminal-process) for more details. |
| 17, 19, 23 | SIGSTOP | Stop | ++ctrl+z++ | Stop execution of a process, but allow it to be resumed through SIGCONT. |

## [Kernel Bypass](https://blog.cloudflare.com/kernel-bypass)

Kernel Bypass is a technology implemented in Linux (and often other kernels as well) that allows network processing to happen in userspace. This often leads to a huge performance improvement for network-bound applications as the traffic does not have to pass through the kernel-userspace boundary.

These are some kernel bypass techniques:

### [PACKET_MMAP](https://www.kernel.org/doc/Documentation/networking/packet_mmap.txt)

Allows the kernel to allocate a circular buffer in userspace so that applications can read their memory directly, instead of making one system call per packet.

### [PF_RING](https://www.ntop.org/products/packet-capture/pf_ring/)

This is a type of network socket, originally implemented by Napatech ntop cards, that provides a circular ring buffer of the network traffic. This is a kernel module that you must load. The kernel module polls packets from the NIC through Linux NAPI and copies the packets from the NIC to the ring buffer, which lives in kernel space. The user application mmaps itself to this kernel buffer. PF_RING is capable of delivering packets to multiple ring buffers, which allows each application to be isolated from others.

### [snabbswitch](https://github.com/snabbco/snabb)

This is a networking framework for Lua applications that allows the app to completely control a network card. The user application acts as a hardware driver. This is done on the PCI device level by mmapping the device registers with sysfs.

### DPDK

A networking framework written in C that is similar to snabbswitch. It also relies on User IO (UIO).

## niceness

"niceness" is a parameter given to processes that determine their overall runtime priority.

### [ionice](https://linux.die.net/man/1/ionice)

`ionice`` determines the IO scheduling priority and class. The various classes that can be used:

- **Idle**: the program with idle IO priority will only get disk time if nothing else is using the disk.
- **Best effort**: This is the default scheduling class. Programs running best-effort are served in a round-robin fashion.
- **Realtime**: The program with realtime class priority will be given first access to the disk. This must be used with care as there is a potential for realtime processes to starve other processes of disk IO.

### [nice](https://man7.org/linux/man-pages/man2/nice.2.html)

`nice` determines the CPU scheduling priority. Processes have values between -20 (highest priority) and 19 (lowest priority).

## CPU Affinity

The `taskset` command is used to set CPU affinity. Example:

```
$ taskset –cp 2 914745
pid 914745’s current affinity list: 0
pid 914745’s new affinity list: 2
```

### Optimizing for the NUMA node

The `lscpu` command will show you which cores are on which NUMA node. If possible, applications should be given CPU affinities that are on a single NUMA node to prevent long-distance memory access on a different node.

## [udev](https://wiki.debian.org/udev)

udev is a replacement for the Device File System (DevFS) starting with the Linux 2.6 kernel series. It allows you to identify devices based on their properties, like vendor ID and device ID, dynamically. udev runs in userspace (as opposed to devfs which was executed in kernel space).

udev allows for rules that specify what name is given to a device, regardless of which port it is plugged into. For example, a rule to always mount a hard drive with manufacturer "iRiver" and device code "ABC" as /dev/iriver is possible. This consistent naming of devices guarantees that scripts dependent on a specific device's existence will not be broken.

```
/etc/udev/rules.d/
```

## Syscalls

### sendfile

`sendfile` is an efficient way to copy data between two file descriptors. Because copying is done in kernel space, it eliminates the context switches needed to userspace in operations that would call `read` then `write`.

A great article by my coworker at Lambda Labs on this topic can be found here: https://www.linuxjournal.com/article/6345

