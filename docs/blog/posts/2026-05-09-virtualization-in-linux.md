---
date: 2026-05-09
categories:
- GPU Virtualization
- System Design
title: "GPU Virtualization Part 3: How Linux Leverages Hardware Isolation"
description: Understanding how Linux leverages hardware for strong isolation, and the various methods to achieve that.
links:
  - blog/posts/2026-02-15-pcie-mmio.md
---

<!-- more -->

## Linux Device Drivers

Most engineers know that drivers are needed to communicate with external devices, but fewer know how drivers are actually allocated MMIO regions to do so, or how the kernel enforces that only the right driver touches the right device.

### MMIO Space Allocation on Boot

When a machine first boots, the BIOS/UEFI firmware enumerates the PCIe bus and determines how much physical address space each device needs according to its [Base Address Registers](2026-02-15-pcie-mmio.md#base-address-registers). You can see these address allocations in the [`/proc/iomem`](2026-02-15-pcie-mmio.md#system-memory-maps-in-linux) file.

When the firmware hands control to the bootloader, and eventually the kernel, the kernel uses a combination of the ACPI tables it receives and device BARs to discover where the firmware created MMIO mappings. It may accept these mappings or create new ones — this behaviour can be controlled through the `pci=realloc` kernel cmdline option.[^3]

However the kernel obtains these mappings, it can communicate with BAR regions by reading from and writing to the allocated physical addresses directly. What each region of the MMIO space _means_ to the device is vendor-defined and unknowable generically. This is where drivers come from: vendor-specific software, running in kernel space, that understands the semantics of each MMIO region and knows how to communicate with the device.

### Driver Attachment

Assume we have a device that was allocated this region of memory: 

```title="/proc/iomem/"
  50001000-500013ff : 0000:00:0f.0
```

A device driver can be given exclusive access to the `50001000-500013ff` memory region by performing a _driver bind_. It looks something like this on the CLI:

```title=""
$ echo '0000:00:0f.0' > /sys/bus/pci/drivers/nvidia/bind
```

Running lspci on the device should show:

```title=""
$ lspci -k -s 65:00.0 | grep -i 'Kernel driver in use'
Kernel driver in use: nvidia
```

Rather than binding devices manually, Linux supports rule-based automation through tools like udev:

```title="/etc/udev/rules.d/99-nvidia-bind.rules"
SUBSYSTEM=="pci", ATTR{vendor}=="0x10de", ACTION=="add", \
  RUN+="/bin/sh -c 'echo nvidia > /sys/bus/pci/devices/%k/driver_override && echo %k > /sys/bus/pci/drivers_probe'"
```

udev is minimal but powerful: it matches PCIe events — like a `0x10de` vendored device appearing in the topology — to scripts that run in response.

You can bind, unbind, and rebind any sequence of drivers you want. Nothing actually changes with the underlying MMIO mappings, the kernel just grants the requested driver access to that region.

### Userspace Drivers

While most device drivers run as kernel-level code, this model has a significant drawback: a single bad driver can crash the entire system. Kernel-mode drivers can access any physical address — process memory, other drivers' state, kernel data structures, all of it.

This is a serious attack surface on multi-tenant systems. A hostile tenant that finds a vulnerability in a kernel driver can potentially gain host-level privileges. This is not theoretical — it has happened in the NVIDIA display driver in [CVE-2021-1052](https://nvd.nist.gov/vuln/detail/CVE-2021-1052).

One solution is to have each tenant run their own driver, bound to their own device, in userspace. Vulnerabilities in a userspace driver cannot directly compromise the kernel. This model is used by:

- [DPDK](https://en.wikipedia.org/wiki/Data_Plane_Development_Kit) for network processing in userspace.
- SPDK for storage processing.
- libibverbs for talking to Infiniband cards.
- VFIO, the most relevant Linux API for GPU virtualization that we'll be talking about more later.

Userspace drivers still need a kernel "driver" that can attach to the device and bridge the gap between kernel-owned MMIO space and userspace. The degree to which a device is driven by userspace versus the kernel varies; some functionality may live in userspace and some in the kernel. The delineation depends on the security requirements and the needs of the application.

### dma-api

The IOMMU code in `drivers/iommu/intel/` is vendor-specific. Exposing it directly to device drivers would require every driver to understand Intel VT-d vs AMD-Vi differences — a non-starter.

The solution is the Linux [dma-api](https://docs.kernel.org/core-api/dma-api-howto.html), which provides device drivers a generic interface for obtaining DMA-mappable addresses regardless of the underlying IOMMU. A short example illustrates the pattern:

```c title=""
#include <linux/dma-mapping.h>

dma_addr_t device_iova;
void *cpu_vaddr;

// 1. Ask the kernel for coherent memory
cpu_vaddr = dma_alloc_coherent(dev, 4096, &device_iova, GFP_KERNEL);

if (cpu_vaddr) {
    // 2. The driver now "knows" the IOVA because it's stored in device_iova.
    // You typically write this IOVA into a hardware register.
    iowrite32(lower_32_bits(device_iova), io_regs + DEVICE_DMA_ADDR_LOW);
    iowrite32(upper_32_bits(device_iova), io_regs + DEVICE_DMA_ADDR_HIGH);
}

dma_free_coherent(dev, size, cpu_vaddr, device_iova);
```

## VFIO

### Giving a Device to Userspace

[Linux Virtual Function IO](https://docs.kernel.org/driver-api/vfio.html), or VFIO for short, is a device driver that is part of the mainline Linux kernel. From the official documentation:

> The VFIO driver is an IOMMU/device agnostic framework for exposing direct device access to userspace, in a secure, IOMMU protected environment. In other words, this allows safe, non-privileged, userspace drivers.

As discussed in the [section above](#userspace-drivers),[^4] in a typical non-virtualized setup a single kernel-mode driver serves all processes on the host. A vulnerability in that driver can compromise the entire host — and this is not theoretical. It has happened repeatedly in the NVIDIA kernel driver, including [CVE-2025-23280](https://nvd.nist.gov/vuln/detail/CVE-2025-23280) and [CVE-2025-23277](https://nvd.nist.gov/vuln/detail/CVE-2025-23277).

VFIO enables a different model: give each tenant their own Linux kernel, and by extension their own isolated kernel-mode device driver. An exploited tenant kernel does not automatically grant access to other tenants.

VFIO also gives applications direct control over IOMMU mappings through an abstraction over Linux's IOMMU drivers:

```c title=""
/* Allocate some space and setup a DMA mapping */
dma_map.vaddr = mmap(0, 1024 * 1024, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
dma_map.size = 1024 * 1024;
dma_map.iova = 0; /* 1MB starting at 0x0 from device view */
dma_map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;

ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);
```

This allocates an anonymous 1 MiB region of memory, populates the `dma_map` struct with its address and size, specifies the starting IOVA, and sends it to VFIO via the `VFIO_IOMMU_MAP_DMA` ioctl. VFIO then calls the appropriate vendor-specific IOMMU driver to install the mapping.

!!! question "What is `ioctl`?"

   [`ioctl`](https://docs.kernel.org/driver-api/ioctl.html) is a special Linux syscall that allows you to send arbitrary data to a specific file handle. The first argument is a _file descriptor_ which more or less represents an instance of a file. A contrived VFIO example might look something like this:

   ```c title=""
   int container;
   container = open("/dev/vfio/vfio", O_RDWR);
   ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);
   ```

### VFIO Groups: The Smallest Safe Unit of Assignment

When the Linux kernel first boots, it walks the PCIe tree and discovers its topology. One concept it tracks is which devices belong to the same IOMMU group. An IOMMU group is a collection of devices the kernel determines cannot be hardware-isolated from each other. For example, if devices A and B belong to group 1 and device C belongs to group 2, A and B can be isolated from C, but not from each other. We can see the groups Linux has created:

Each IOMMU group appears as a numbered directory under `/sys/kernel/iommu_groups/`, with a `devices/` subdirectory containing symlinks to every device in that group:

```title=""
$ ls /sys/kernel/iommu_groups/
0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17

$ ls /sys/kernel/iommu_groups/16/devices/
0000:65:00.0  0000:65:00.1
```

A more useful view pairs each group with `lspci` output:

```title=""
$ for d in /sys/kernel/iommu_groups/*/devices/*; do
    group=$(echo "$d" | cut -d/ -f5)
    bdf=$(basename "$d")
    printf "Group %3d: %s\n" "$group" "$(lspci -s "$bdf" | head -1)"
  done | sort -k2 -n
Group   0: 0000:00:00.0 Host bridge: Intel Corporation ...
Group   1: 0000:00:01.0 PCI bridge: Intel Corporation ...
...
Group  16: 0000:65:00.0 3D controller: NVIDIA Corporation A100 80GB PCIe [10de:20b2]
Group  16: 0000:65:00.1 Audio device: NVIDIA Corporation GA100 High Definition Audio Controller
```

Notice that both the GPU and its associated audio function share group 16. Because the kernel cannot isolate these two devices from each other, VFIO requires you to assign the entire group together — you cannot pass through `65:00.0` without also handling `65:00.1`.

Linux considers two devices to be in separate groups if a packet must pass through an ACS-capable device to get from one to the other. Often, an "ACS-capable device" is just a switch, but it could also include other devices like regular bridges or root ports. This distinction is critical to track, because if two devices can route packets without ACS-capable devices, it means their packets can't be forced upstream to the IOMMU, which means they will always be able to talk directly to each other.

Why would we care about this? Consider if we're on a system where two GPUs `0000:00.01.0` and `0000:00:02.0` can talk to each other without ever traversing an ACS-capable device:

```title=""
                [ CPU Root Complex ]
                         |
                 [ PCIe Root Port ]
                         |
                 [ PCIe Switch ]   <-- No ACS support
                   /         \
                  /           \
[ GPU 0 (0000:00:01.0) ]   [ GPU 1 (0000:00:02.0) ]
```

If we assign GPU 0 to VM A and GPU 1 to VM B, these two devices can speak directly to each other if they know their MMIO addresses, which completely breaks tenant isolation. The VFIO system specifically forbids this from happening because it's unsafe and breaks core assumptions. VFIO exposes its groups as character devices (cdevs) like this:

```title=""
$ ls /dev/vfio/
/dev/vfio/vfio1
/dev/vfio/vfio2
```

If one virtual machine mounts a VFIO group to a VFIO container like this:

```c title=""
char group_path[128] = "/dev/vfio/vfio1"
int container = open("/dev/vfio/vfio", O_RDWR);
int group = open(group_path, O_RDWR);
if (ioctl(group, VFIO_GROUP_SET_CONTAINER, &container) < 0) {
   printf("VFIO_GROUP_SET_CONTAINER failed: %s\n", strerror(errno));
   printf("\nThis is the expected failure when the group is not viable.\n");
   return EXIT_FAILURE;
}
```

Once a VFIO group is assigned to a container, any attempt to assign it to a different container will fail.

### VFIO Containers and iommufd

So far, we’ve talked about three key pieces:

- The IOMMU, which enforces memory isolation
- VFIO groups, which define the smallest safe unit of device assignment
- VFIO itself, which exposes devices and IOMMU control to userspace

Now we need to connect these pieces into something usable. That connection point is the VFIO container. 
#### The Container as an Address Space

A VFIO container represents a single IOMMU address space from userspace’s perspective: the IOMMU domain is the hardware construct, and the container is the userspace handle to it.

When you create a container:

```c title=""
int container = open("/dev/vfio/vfio", O_RDWR);
```

you're effectively asking the kernel for a control handle for an IOMMU-backed address space. At this point, the container is empty. It has no devices, no mappings, no IOMMU domains yet. 

#### Attaching Groups to a Container

The next step is to attach a VFIO group:

```c title=""
ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);
```

This operation does two important things:

1. It associates the group’s devices with the container
2. It ensures those devices will share the same IOMMU domain

This is where things become concrete: A VFIO container with one or more VFIO groups represents a single IOMMU domain. All of the devices in those groups will use the same page tables, share the same IOVA space, and be able to DMA into the same memory mappings. This is what you want for something like a virtual machine where multiple devices (GPU, NIC, etc.) need to operate within the same memory space.

#### Programming the IOMMU Through the Container

Once a group is attached, the container can be used to program DMA mappings: 

```c title=""
ioctl(container, VFIO_IOMMU_MAP_DMA, &dma_map);
```

At this point, VFIO does the heavy lifting:

- It translates your request into vendor-specific IOMMU operations
- It installs mappings into the underlying IOMMU domain
- It ensures permissions (read/write) are enforced

From the device’s perspective, it now sees a valid IOVA space. From the system's perspective, you have just defined the exact memory this device is allowed to touch. Everything outside that mapping is inaccessible.

#### Why Containers Exist at All

You might reasonably ask: why not just attach devices directly to IOMMU domains? The answer is composability. VFIO containers allow:

- Multiple groups → one domain (e.g., GPU + NIC for one VM)
- Userspace ownership of the address space
- A clean abstraction over wildly different IOMMU implementations

Without containers, every userspace application would need to understand Intel VT-d vs AMD-Vi differences, manage page tables directly, and handle device grouping constraints manually. The container hides all of that behind a simple interface.

#### iommufd: The Modern Replacement

The VFIO container model works, but it has limitations. The first was that Linux provided multiple IOMMU-backed passthrough frameworks, another being vDPA, that implemented redundant IO page table logic. VFIO also struggled to support more advanced hardware features like PASID, nested translations, and IO page fault delivery to userspace. This called for a new, unifying model for IOMMU management. This led to the introduction of [iommufd](https://docs.kernel.org/userspace-api/iommufd.html).

Its primary goals:

- **Unified framework**: a single shared API replaces the per-framework IOMMU abstractions that VFIO, vDPA, and others each implemented independently.
- **Advanced hardware features**: native support for PASID, nested translations, and IO page fault delivery to userspace — features VFIO could not express.
- **Device-centric model**: VFIO's group-centric model made it awkward to target individual devices; iommufd adopts a [device-centric model](https://qemu.readthedocs.io/en/master/devel/vfio-iommufd.html) instead.
- **Reduced overhead**: more direct interaction with IOMMU hardware reduces VM exits and hypercalls for high-speed I/O.
- **Better page table performance**: algorithmic improvements to iommufd's [page table management](2025-11-19-gpu-passthrough-boot.md#is-iommufd-faster-than-legacy-vfio-backend) reduce mapping overhead.


Instead of implicitly tying everything to a VFIO container, iommufd introduces explicit objects:

- IO address spaces (IOAS): represent IOMMU domains
- Hardware page tables (HWPT): represent translation structures
- Device bindings: explicitly attach devices to those domains

Conceptually, this is a more direct mapping to hardware:

```title=""
Userspace
   │
   ▼
iommufd IOAS  ───>  IOMMU Domain
   │
   ▼
Device Bindings ───> PCIe Devices
```

The VFIO API has support for iommufd through the `VFIO_DEVICE_BIND_IOMMUFD` and `ATTACH_IOAS` ioctls.

??? tip "Advanced Example"

      You can see the main IOMMUFD cdev here:

      ```title=""
      $ ls -l /dev/iommu
      crw------- 1 root root 10, 125 Apr 24 12:34 /dev/iommu
      ```

      You can then find the IOMMUFD file of a specific PCIe device (taken from the vfio docs):

      ```title=""
      $ ls /sys/bus/pci/devices/0000:6a:01.0/vfio-dev/
      vfio0
      $ ls -l /dev/vfio/devices/vfio0
      crw------- 1 root root 511, 0 Feb 16 01:22 /dev/vfio/devices/vfio0
      $ cat /sys/bus/pci/devices/0000:6a:01.0/vfio-dev/vfio0/dev
      511:0
      $ ls -l /dev/char/511\:0
      lrwxrwxrwx 1 root root 21 Feb 16 01:22 /dev/char/511:0 -> ../vfio/devices/vfio0
      ```

      You can then get a file descriptor like this:

      ```c title=""
      cdev_fd = open("/dev/vfio/devices/vfio0", O_RDWR);
      ```

      And use that descriptor to bind to an iommufd like this:

      ```c title=""
      struct vfio_device_bind_iommufd bind = {
         .argsz = sizeof(bind),
         .flags = 0,
      };

      int dev_fd = open("/dev/vfio/devices/vfio0", O_RDWR);
      int iommufd = open("/dev/iommu", O_RDWR);
      bind.iommufd = iommufd;

      ioctl(dev_fd, VFIO_DEVICE_BIND_IOMMUFD, &bind);
      ```


### What Happens During GPU Passthrough


## References

- https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-04-24-virtualization-in-linux/lp1467.pdf
- https://www.youtube.com/watch?v=kl9c6DrDnHo
- https://github.com/Johannes4Linux/Linux_Driver_Tutorial_legacy


[^1]: You can already probably tell that different styles of Security Domains provide different guarantees. SDs that co-reside on physical hardware, although for all intents and purposes reasonably isolated from each other, are still vulnerable to hardware-based side channel attacks.
[^2]: It talks to the IOMMU through MMIO just like any other ordinary device. PCIe devices and other peripheral components like the IOMMU can all share the same physical address space. When Intel VT-d is turned on (or AMD's equivalent), the BIOS firmware initializes the IOMMU and provides its MMIO address to the OS through the ACPI DMAR table.
[^3]: It's usually a good idea to let the kernel reallocate MMIO mappings, especially when you're dealing with GPU virtualization, because the firmware often will not allocate enough space for large BAR devices. It's common to run into strange GPU driver errors if this is option isn't provided because the driver often won't have access to the entire BAR regions it needs. I haven't determined the exact reason why this happens, but it's likely some historic limitation or assumption of BIOS firmware around how large BAR devices are handled.
[^4]: A driver that's running in a virtual machine could also be considered a userspace driver, at least from the perspective of the host.
