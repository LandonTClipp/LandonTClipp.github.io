---
date: 2026-05-08
categories:
- GPU Virtualization
- System Design
title: "GPU Virtualization Part 2: IOMMUs and Hardware Isolation"
description: A focused look into how IOMMUs work and why it's critical for isolating PCIe fabrics.
links:
  - blog/posts/2026-02-15-pcie-mmio.md
---

![](https://f005.backblazeb2.com/file/landons-blog/assets/posts/2026-05-08-iommu-and-hardware-isolation/gpu-virt-part2-hero-princeton-wide.png){ loading=lazy style="object-fit: cover;"}

Most engineers treat virtualization as a software abstraction problem. It isn’t. Isolation is fundamentally a hardware problem — software can request it, but hardware is the only thing that can enforce it. If the underlying hardware allows two components to communicate, no amount of abstraction can fully prevent it.

This becomes especially true with PCIe devices like GPUs. Unlike processes or threads, devices do not understand memory ownership, privilege boundaries, or tenants. They issue DMA reads and writes directly against physical memory. Without the right hardware mechanisms in place, a device can DMA into any physical address on the system — including another tenant’s memory or the kernel itself. If you’re responsible for running multiple tenants on shared GPU hardware, whether your isolation is hardware-enforced or just a promise is the question that matters most.

In the [previous post](2026-02-15-pcie-mmio.md), we explored how the PCIe fabric moves data and how MMIO regions are established. That foundation raises one concrete question:

> How does Linux use hardware to actually enforce isolation?

To answer it, we’ll introduce the concept of a **Security Domain** and then examine how the IOMMU enforces it in hardware. How Linux software exposes these boundaries to userspace and VMs — through VFIO and iommufd — is covered in the next post.

<!-- more -->

## Security Domains

Before we talk about virtualization, we need a clear definition of what we’re actually trying to build. It all reduces to one question:

> “What is allowed to talk to what?”

**Security Domain (SD)**

A Security Domain is a boundary enforced by hardware such that anything inside the domain cannot directly access anything outside of it.

That’s it. Everything else in virtualization is just an implementation detail of this idea.

At one extreme, two physically separate machines are two different security domains. There is no communication path between them, so they are perfectly isolated.

```title=""

                                                                                 
                                                                                 
                                       |                                         
          Bare Metal                   |                  Bare Metal             
                                       |                                         
+---------------------------------+    |      +---------------------------------+
|       Security Domain A         |    |      |       Security Domain B         |
|                                 |    |      |                                 |
|                                 |    |      |                                 |
|                                 |    |      |                                 |
| +----------+       +----------+ |    |      | +----------+       +----------+ |
| |Process A |       |Process B | |    |      | |Process C |       |Process D | |
| |          +------>|          | |    |      | |          +------>|          | |
| |          |<------+          | |    |      | |          |<------+          | |
| |          |       |          | |    |      | |          |       |          | |
| +----------+       +----------+ |    |      | +----------+       +----------+ |
|                                 |    |      |                                 |
|                                 |    |      |                                 |
|                                 |    |      |                                 |
+---------------------------------+    |      +---------------------------------+
                                       |                                         
                                       |                                         
                                       |                                         
```

Move down the isolation spectrum and you get a more common scenario: two VMs sharing the same physical host. Physical separation is gone, so isolation must be actively enforced — the IOMMU confines device DMA to tenant-owned memory, the MMU enforces per-process virtual address spaces, ACS-capable PCIe switches block peer-to-peer transactions between tenant devices, and CPU features limit side-channel leakage between cores. This is more complex than physical separation, but has been a largely solved problem for some decades — notwithstanding hardware vulnerabilities like the infamous [Meltdown and Spectre sidechannel attacks](https://meltdownattack.com/).[^1]

```title=""
+--------------------------------------------------------------------------+
|                             Bare Metal                                   |
|                                                                          |
|                             +-------------+                              |
|                             |   IOMMU     |                              |
|                +----------->|   ACS       |<---------+                   |
|                |            |   MMU etc.  |          |                   |
|                |            +-------------+          |                   |
| +--------------+------------------+  +---------------+-----------------+ |
| |       Security Domain A         |  |       Security Domain B         | |
| |            (VM)                 |  |            (VM)                 | |
| |                                 |  |                                 | |
| |                                 |  |                                 | |
| | +----------+       +----------+ |  | +----------+       +----------+ | |
| | |Process A |       |Process B | |  | |Process C |       |Process D | | |
| | |          +------>|          | |  | |          +------>|          | | |
| | |          |<------+          | |  | |          |<------+          | | |
| | |          |       |          | |  | |          |       |          | | |
| | +----------+       +----------+ |  | +----------+       +----------+ | |
| |                                 |  |                                 | |
| |                                 |  |                                 | |
| |                                 |  |                                 | |
| +---------------------------------+  +---------------------------------+ |
+--------------------------------------------------------------------------+
```

These two scenarios share an important structural property: the outer security domain — the bare metal host — is owned by the cloud provider. The tenant’s VM lives inside it. This has a direct consequence for GPU virtualization. When the provider uses the host IOMMU to isolate tenant VMs, that hardware is consumed by the outer layer. A tenant that wants to further subdivide devices among their own workloads faces a narrow path to hardware-backed isolation — nested IOMMU translation exists in hardware but software support for exposing it to guests is still immature. In practice, most are forced to fall back on software-based device isolation. This is the primary reason GPU virtualization products operate under weaker isolation models than bare metal: not by choice, but because the hardware isolation machinery was already spoken for. Every technique in this space trades isolation strength for performance or hardware availability — a tension that runs through everything that follows.

## How the IOMMU Builds a Device Security Domain

The [previous post](2026-02-15-pcie-mmio.md) established what the IOMMU does: it translates device-visible addresses into host physical addresses. Here we look at how it does it — specifically, how the IOMMU uses that translation layer to place a PCIe device inside a hardware-enforced memory security domain. The IOMMU enforces this by forcing every device DMA through a translation it controls.

Instead of allowing a device to say:

> “write to host physical address `0x1234...`”

the system makes the device say:

> “write to device-visible address `0xabc...`”

The IOMMU then decides what host physical address, if any, that device-visible address is allowed to resolve to. This is the core trick: the device gets its own address space. Linux calls this an **IOMMU domain**.

An IOMMU domain is conceptually similar to a process address space, except it is used for DMA instead of CPU loads and stores. It describes which host physical pages a device is allowed to access, and what device-visible addresses map to those pages.

```text title=""
Device IOVA --IOMMU translation--> Host Physical Address
```

If a device tries to DMA to an address that is not mapped in its IOMMU domain, the IOMMU can block the access and raise a fault. That is the beginning of hardware-backed device isolation.

### Devices Need an Identity

For the IOMMU to enforce isolation, it needs to know which device sent a transaction.

On PCIe, transactions carry a requester identity. In Linux, we usually talk about this in terms of the device’s PCI Bus-Device-Function, or BDF:

```title=""
0000:65:00.0
```

The BDF is the device’s identity on the PCIe fabric. In the Linux kernel, it is encoded as a single integer from the bus number and device/function fields:

??? tip "pci_dev_id in the kernel"

    ```c title="https://github.com/torvalds/linux/blob/ac2dc6d57425ffa9629941d7c9d7c0e51082cb5a/include/linux/pci.h#L739-L742"
    static inline u16 pci_dev_id(struct pci_dev *dev)
    {
    	return PCI_DEVID(dev->bus->number, dev->devfn);
    }
    ```

This identity matters because the IOMMU needs to answer a simple question for every DMA transaction: which IOMMU domain does this device belong to? That lookup is performed through a set of hardware-defined tables.

### Root Entries, Context Entries, and Domains

On Intel VT-d systems, the IOMMU uses a hierarchy of tables to map a requester ID to an address translation structure. AMD-Vi uses a conceptually equivalent design — a Device Table maps requester IDs directly to I/O Page Table roots — so the same model applies regardless of vendor.

At a high level, the lookup looks like this:

```title=""
Requester ID
   |
   v
Root Entry
   |
   v
Context Entry
   |
   v
IOMMU Domain / Page Tables
   |
   v
Host Physical Address
```

The root table is the first level of this lookup. Each entry is a 16-byte `root_entry` containing a present bit and a pointer to a context table:

??? tip "root_entry in the kernel"

    ```c title="https://github.com/torvalds/linux/blob/ac2dc6d57425ffa9629941d7c9d7c0e51082cb5a/drivers/iommu/intel/iommu.h#L544-L553"
    /*
     * 0: Present
     * 1-11: Reserved
     * 12-63: Context Ptr (12 - (haw-1))
     * 64-127: Reserved
     */
    struct root_entry {
    	u64     lo;
    	u64     hi;
    };
    ```

A root entry points to a context table.

The context table contains `context_entry` structures. Each describes how transactions from a particular device/function should be translated — specifically, which page tables to use and what IOMMU domain the device belongs to:

??? tip "context_entry in the kernel"

    ```c title="https://github.com/torvalds/linux/blob/ac2dc6d57425ffa9629941d7c9d7c0e51082cb5a/drivers/iommu/intel/iommu.h#L555-L569"
    /*
     * low 64 bits:
     * 0: present
     * 1: fault processing disable
     * 2-3: translation type
     * 12-63: address space root
     * high 64 bits:
     * 0-2: address width
     * 3-6: aval
     * 8-23: domain id
     */
    struct context_entry {
    	u64 lo;
    	u64 hi;
    };
    ```

The three fields that matter:

- **Address space root**: points to the page tables for this device’s IOVA space
- **Domain ID**: identifies which IOMMU domain this device belongs to
- **Translation type**: tells the IOMMU how to interpret the translation structure

The context entry is the moment the IOMMU learns which address space a device belongs to. From here, it walks the page tables and translates the device’s IOVA into a host physical address.

### The Lookup Path

When a PCIe device performs DMA, the process unfolds as follows:

1. The device emits a PCIe transaction containing a requester ID and a device-visible address (the IOVA).
2. The IOMMU uses the requester ID to find the correct context entry.
3. The context entry tells the IOMMU which domain and page-table root apply.
4. The IOMMU walks the page tables for that domain.
5. If the mapping exists and permissions allow it, the IOMMU translates the IOVA into a host physical address.
6. If the mapping does not exist, the IOMMU blocks the transaction and reports a fault.

That is the whole security story in miniature. The device is not trusted to choose physical addresses directly. It is only trusted to issue addresses inside the address space Linux gave it.

### Caching: Context Cache and IOTLB

Walking IOMMU tables for every DMA transaction would be far too slow, so the IOMMU caches the results.

There are two important caches to know about:

1. Context cache: Caches the relationship between a requester ID and its context entry, keyed by domain ID. Once cached, the IOMMU skips the root and context table walks entirely.
2. IOTLB: Caches individual IOVA-to-host-physical-address translations.

The ideal fast path looks like this:

```title=""
PCIe transaction arrives
   |
   v
Requester ID hits in context cache
   |
   v
(domain ID, IOVA) hits in IOTLB
   |
   v
Translated host physical address
```

In this fast path, no table walks occur — the translation is served entirely from on-chip caches.

This is critical for high-throughput devices like GPUs and NICs. DMA-heavy devices can issue enormous numbers of memory transactions. If every transaction required a full table walk, IOMMU translation overhead would be catastrophic.

### Why This Matters for GPU Virtualization

For GPU virtualization, this machinery is not an implementation detail. It is the isolation boundary. A passed-through GPU, virtual GPU (vGPU), or other PCIe-attached accelerator must be prevented from issuing DMA into memory owned by the host or another tenant. The IOMMU domain is the hardware-enforced box around that device.

When Linux assigns a device to a VM through VFIO, this is the fundamental operation being performed:

> Put this device into an IOMMU domain whose mappings correspond only to memory the VM is allowed to access.

That is what makes device passthrough viable in a hostile multi-tenant environment. Without the IOMMU, passthrough is not strong isolation. It is trust. And in cloud infrastructure, trust is not an isolation strategy.

### An End-to-End Diagram

Below is a full end-to-end diagram tracing the slow path — a cache miss where neither the context cache nor the IOTLB has a warm entry for this device.

```title=""
                                                                  +---------------------------------------------+
                                                                  |                Host Memory                  |
                                                                  |   (4)                                       |
                                                            +-----+-------------------+                         |
                                                            |     |                   |                         |
                                                            |     |                   v                         |
+--------------+         +-------------------------------+  |     |                 +---+---+---+---+---+--+--+ |
|              |         |            IOMMU     (3)      |  |     | root_entry[256] |   |   |   |   |   |  |  | |
|              |  (1)    |(2)       +------------------->+--+     |                 |   |   |   |   |   |  |  | |
|PCIe Device   +-------->+----+     |                    |        |                 +-+-+---+---+---+---+--+--+ |
|              |         |    |     |                    |        |                   |                         |
|              |         |    v     |                    |        |                   +---+(5)                  |
+--------------+         | +--------+---+      (7)       |        |                       |                     |
                         | |            |<-------------+ |        |                 +---+-v-+---+---+---+--+--+ |
                         | |            |              | |        | context_entry[n]|   |   |   |   |   |  |  | |
                         | |            |              | |        |                 |   |   |   |   |   |  |  | |
                         | |            | +-------+    | |        |                 +---+-+-+---+---+---+--+--+ |
                         | |   Context  | |IOTLB  |    | |        |       (6)             |                     |
                         | |   Cache    | |       |    +-+<-------+-----------------------+                     |
                         | |            | |       |      |        |     (8)                                     |
                         | |            | |       |      +--------+-----------------+                           |
                         | |            | |       |      |        |                 |                           |
                         | +------------+ +-------+      |        |                 v-------------------------+ |
                         +-------------------------------+        | page tables     |                         | |
                                                                  |                 +-------------------------+ |
                                                                  +---------------------------------------------+
```

1. A TLP arrives from a device into the IOMMU.
2. The IOMMU uses the device's BDF to determine whether its context has already been cached.
3. The context has not been cached.
4. The IOMMU finds the `root_entry` element.[^2] The device's bus number is used as the index into the root table.
5. The `root_entry` element that we found then points to the start of the `context_entry` table. The Device-Function portion of the BDF is used as an index into this table. 
6. The `context_entry` value is returned to the IOMMU. 
7. The IOMMU caches the value of this `context_entry` element in its `Context Cache`.
8. Using the now-cached `context_entry`, the IOMMU uses bits 12-63 of the struct to find the starting address of the page table structure.

## IOMMU Groups

Not every device lands in its own IOMMU domain. Linux enforces isolation at the granularity of **IOMMU groups** — sets of devices that cannot be isolated from each other given the current PCIe topology.

Two situations force devices into the same group:

**Multi-function devices.** A single PCIe device can expose multiple logical functions under the same BDF prefix. A GPU typically exposes a compute function and an audio controller as two separate functions. Because both functions belong to the same physical chip and share internal logic, they cannot be independently isolated — they always land in the same group.

**PCIe switches without ACS.** Devices behind the same PCIe switch can issue peer-to-peer (P2P) DMA — transactions that travel directly from one device to another without passing through the Root Complex or the IOMMU. Because P2P traffic never reaches the IOMMU, it cannot be subject to domain enforcement. **Access Control Services (ACS)** is a PCIe feature that instructs switches to redirect all transactions up through the Root Complex, where the IOMMU can inspect them. Without ACS, every device behind the same switch ends up in the same IOMMU group regardless of which domains they're assigned to.

The IOMMU group is the minimum unit of isolation Linux can enforce. It will not let you assign individual devices from the same group to different VMs — you must assign the entire group or none of it.

```title=""
IOMMU Group 16
  +-- 0000:65:00.0  NVIDIA A100 (GPU compute function)
  +-- 0000:65:00.1  NVIDIA GA100 Audio Controller (GPU audio function)
```

You can inspect IOMMU groups on a Linux host:

```bash title=""
$ for d in /sys/kernel/iommu_groups/*/devices/*; do
    group=$(echo "$d" | cut -d/ -f5)
    bdf=$(basename "$d")
    printf "Group %3d: %s\n" "$group" "$(lspci -s "$bdf" | head -1)"
  done | sort -k2 -n
Group  16: 0000:65:00.0 3D controller: NVIDIA Corporation A100 80GB PCIe [10de:20b2]
Group  16: 0000:65:00.1 Audio device: NVIDIA Corporation GA100 High Definition Audio Controller
```

On well-configured servers where each GPU sits behind an ACS-capable PCIe switch, each GPU occupies its own group and passthrough works cleanly. On systems without ACS, GPUs can land in groups alongside unrelated devices, and passing through the GPU means passing through those devices too — which is usually unacceptable in a multi-tenant environment.

ACS support in the PCIe fabric is therefore a prerequisite, not a nice-to-have, for providers who need per-device isolation.

## What Happens Without the IOMMU?

Without a hardware IOMMU, either because VT-d wasn't enabled, the hardware doesn't support it, or you're inside a VM without access to the host IOMMU, the PCIe Root Complex operates in _IOMMU Bypass_ Mode. In bypass mode, the Root Complex skips IOVA -> HPA translation entirely and treats device-visible addresses as physical addresses directly.

The consequences are severe: every device in the PCIe fabric can interact with _any_ physical address, DMA or MMIO. If you plug two tenants into this system, **a malicious device could read and write another tenant’s memory**, or even the kernel itself. That's scary.

### VMs Struggle to Create Performant Security Domains

This segues into another point worth understanding. When a VM is provided a device using DMA remapping on the host via an IOMMU, the question arises: can the VM itself create child IOMMU domains for further isolation?

Modern IOMMU hardware actually does support this through nested translation. Intel VT-d's Scalable Mode (introduced in VT-d 3.0) and AMD-Vi both support two-level IOMMU translation — analogous to how nested page tables (Extended Page Tables on Intel) work for CPU memory virtualization. In this model, the guest manages a first-level translation table while the host retains a second-level table, and the hardware walks both on each DMA access.

The catch is that software support for actually exposing this capability to a guest VM is still maturing. Full end-to-end support requires cooperation from the VMM, the guest kernel, and the host IOMMU driver. The Linux kernel's `iommufd` subsystem is the primary vehicle for this and has been gaining nested translation support since ~6.7, but it is not yet a widely deployed capability. In practice, most VMs that need to create child security domains fall back to a software-emulated IOMMU, or [vIOMMU](https://wiki.qemu.org/Features/VT-d), implemented by the VMM (e.g., QEMU, cloud-hypervisor).

Software vIOMMUs are slow, since every address translation is a trap into the VMM rather than a hardware page table walk. Even when hardware nested translation is used, it still incurs overhead compared to flat single-level translation: every DMA access requires the hardware to walk two levels of page tables instead of one. For GPU workloads, where DMA is high-frequency and latency-sensitive, either approach carries enough overhead that nested IOMMU isolation within a VM is effectively ruled out as a viable option.

## Looking Forward

The IOMMU is the hardware primitive that makes device isolation possible. It answers the question "can this device access this memory?" by construction — either a mapping exists in the device's IOMMU domain, or the transaction is blocked.

But hardware primitives don't configure themselves. The IOMMU needs software to build domain tables, assign devices to domains, map guest memory, and expose all of this safely to userspace and VMs. That is the job of the Linux device driver stack — and, for device passthrough specifically, of VFIO and iommufd.

The next post in this series will pick up where this one ends: how Linux software takes the hardware isolation machinery described here and turns it into a usable interface for virtualizing PCIe devices.

[^1]: You can already probably tell that different styles of Security Domains provide different guarantees. SDs that co-reside on physical hardware, although for all intents and purposes reasonably isolated from each other, are still vulnerable to hardware-based side channel attacks.
[^2]: VT-d supports up to 256 root entries because the root table fits in a single 4 KiB page: 4096 bytes ÷ 16 bytes per `root_entry` = 256 entries. The device's bus number (8 bits, values 0–255) serves as the index. 