---
title: PCIe
---


## [Lanes](https://en.wikipedia.org/wiki/PCI_Express#Lane)

Each lane in a PCIe bus is composed of two differential signal pairs. One pair receives data and the other transmits. Thus, each PCIe lane consists of four signal wires. The use of a differential signal pair where the signal is mirrored 180 degrees on each wire in the pair, as opposed to a single signal wire, is a common [noise reduction technique](https://en.wikipedia.org/wiki/Differential_signalling). 

## Switch

A PCIe switch shuttles packets on the PCIe bus to different endpoints. It works in much the same way as a networking switch.

## Bridge

A PCIe bridge is a device that allows communication across two different PCIe busses.

## ATS

![ATS Block Diagram](https://community.cadence.com/resized-image/__size/926x608/__key/communityserver-blogs-components-weblogfiles/00-00-00-00-11/4834.pastedimage1709589624935v2.png)

The ATS, or Address Translation Services, supports translation DMA (Direct Memory Access) addresses to addresses on the PCIe device. It is comprised of many different sub-components that work together to provide fast, low-latency address translation.

The ATS relieves the CPU from having to perform these address translations itself. It runs as a set of hardware components that bypass the CPU.

### Translation Agent

The Translation Agent is a software component running on the host that translates addresses on behalf of the host for PCIe devices. It uses data structures like page tables and Translation Lookaside Buffers (TLB) to perform the virtual-to-physical memory translations.

### Address Translation Cache

The ATC is a cache on the device that stores translations between virtual and physical addresses. This cache stores results from the TA and allows translations to be performed with much lower latency.

### Address Translation Protection Table

The ATPT is a data structure used to store page tables for an address translation. It contains the set of address translations accessed by a Translation Agent to process PCIe requests.

References:

- https://www.intel.com/content/www/us/en/docs/programmable/683686/20-4/address-translation-services-ats.html
- https://community.cadence.com/cadence_blogs_8/b/fv/posts/navigating-the-complexity-of-address-translation-verification-in-pci-express-6-0

## [Root Complex](https://en.wikipedia.org/wiki/Root_complex)

![Root Complex diagram](https://upload.wikimedia.org/wikipedia/commons/1/1c/Example_PCI_Express_Topology.svg)

A Root Complex device connects the CPU and memory subsystem to the PCIe switch fabric. It generates transaction requests on behalf of the CPU, which is interconnected through a local bus. Root Complex functionality may be integrated in the chipset and/or the CPU.

All CPU<->Memory operations happen inside of the root complex. The CPU may attempt to access something on the PCIe bus, in which case the root complex will forward the request to the PCIe controller, from where the controller will route the request to the proper PCIe endpoint.

References:

- https://electronics.stackexchange.com/questions/461251/what-is-the-role-of-the-root-complex-in-a-microprocessor-system-pci-express


## `sysfs`

You can interact with devices on the PCIe bus using `sysfs`. Details listed [here](/sysadmin/linux/#sysfs).

### Removing Device from PCI

`sysfs` can be used to remove a specific PCIe device. If a GPU is being used by a VM, for example, we need to first remove the VM with `virsh destroy`.

Then, we perform:

```
echo 1 > /sys/bus/pci/devices/${address}/remove
echo 1 > /sys/bus/pci/rescan
```

According to [`kernel.org`](https://docs.kernel.org/PCI/sysfs-pci.html):

!!! quote

    The 'remove' file is used to remove the PCI device, by writing a non-zero integer to the file. This does not involve any kind of hot-plug functionality, e.g. powering off the device. The device is removed from the kernel's list of PCI devices, the sysfs directory for it is removed, and the device will be removed from any drivers attached to it. Removal of PCI root buses is disallowed.


## CLI Tools

### lspci

```
$ lspci
00:00.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Starship/Matisse Root Complex
00:00.2 IOMMU: Advanced Micro Devices, Inc. [AMD] Device 164f (rev 01)
00:01.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Starship/Matisse PCIe Dummy Host Bridge
```

#### Filter by Vendor

[PCI-SIG maintains a list of vendor IDs](https://pcisig.com/membership/member-companies?combine=&order=field_vendor_id&sort=asc) that are used when reporting the vendor to the PCI system.

For example, you can query all NVIDIA devices using the vendor ID `0x10de`:

```
$ lspci -v -d 10de:
18:00.0 3D controller: NVIDIA Corporation Device 2330 (rev a1)
2a:00.0 3D controller: NVIDIA Corporation Device 2330 (rev a1)
```


### setpci

```
NAME
       setpci - configure PCI devices

SYNOPSIS
       setpci [options] devices operations...

DESCRIPTION
       setpci is a utility for querying and configuring PCI devices.

       All numbers are entered in hexadecimal notation.

       Root privileges are necessary for almost all operations, excluding reads of the standard header of the configuration space on some operating systems.  Please see lspci(8) for details on access rights.
```

