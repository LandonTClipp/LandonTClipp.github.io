---
title: PCIe
---


## [Lanes](https://en.wikipedia.org/wiki/PCI_Express#Lane)

Each lane in a PCIe bus is composed of two differential signal pairs. One pair receives data and the other transmits. Thus, each PCIe lane consists of four signal wires. The use of a differential signal pair where the signal is mirrored 180 degrees on each wire in the pair, as opposed to a single signal wire, is a common [noise reduction technique](https://en.wikipedia.org/wiki/Differential_signalling). 

## [Link Training](https://www.intel.com/content/www/us/en/docs/programmable/683647/18-0/link-training.html)

The physical layer of PCI performs link training, which refers to a handshake that two PCI endpoints perform with each other to negotiate the parameters of the connection. Such parameters include things like link width, speed, and equalization settings.

The link training handshake is handled by the Link Training and Status State Machine (LTSSM). This state machine ensures that each partner in the negotiation is progressing normally.

??? chatgpt "ChatGPT 4o"

    The PCIe (Peripheral Component Interconnect Express) link training process is a critical step during the initialization of PCIe connections between two devices. This process ensures that the link is established correctly, operating at the highest possible speed and width that both devices and the physical link can support reliably. Here is a detailed description of the PCIe link training process:

    ### Overview of PCIe Link Training:

    1. **Detection Phase**:
    - The process begins with the detection of a connected device. The Root Complex (RC) or upstream device and the Endpoint (EP) or downstream device detect each other's presence through electrical signals on the link.
    
    1. **Polling Phase**:
    - The devices transition into a polling state to establish communication parameters. They exchange training sets to synchronize and align their clocks. This phase ensures that both devices are ready to proceed with the link training.

    1. **Configuration Phase**:
    - During this phase, the devices determine the capabilities of the link, including the number of lanes and the supported data rates. They start with the lowest data rate (e.g., 2.5 GT/s for PCIe Gen1) and the maximum number of lanes physically available.
    - The devices negotiate and confirm the lane configuration and data rate. If the initial configuration is successful, they may attempt to train at higher speeds.

    1. **Link Training and Status State Machine (LTSSM)**:
    - The Link Training and Status State Machine (LTSSM) manages the state transitions during the link training process. The LTSSM defines various states, such as Detect, Polling, Configuration, and L0 (active state), and controls the flow between these states to ensure a stable link.

    1. **Equalization**:
    - Equalization is a critical step to compensate for signal degradation over the physical link. The devices adjust their transmitter and receiver parameters to optimize signal quality and minimize errors. This may involve tuning parameters such as pre-emphasis and de-emphasis.
    - Equalization typically occurs in multiple phases (Phase 1, Phase 2, and Phase 3) to fine-tune the signal characteristics for reliable high-speed data transfer.

    1. **Training Sets Exchange**:
    - The devices exchange training sets, which are special data patterns used to test and adjust the link parameters. These training sets help in calibrating the link and ensuring that data can be transmitted and received correctly.

    1. **Error Detection and Correction**:
    - During the training process, error detection mechanisms (such as Cyclic Redundancy Check or CRC) are used to identify and correct any errors in the communication. If errors are detected, the devices may re-initiate parts of the training process to correct the issues.

    1. **Link Up (L0 State)**:
    - Once the link parameters are successfully negotiated and optimized, the devices transition to the L0 state, which is the active state where normal data transfer operations occur. In this state, the link is fully operational, and the devices can communicate at the negotiated speed and lane width.

    ### Detailed LTSSM States:

    1. **Detect**: The link is powered up, and the presence of a connected device is detected through electrical signals.
    2. **Polling**: The devices exchange training sets to synchronize their clocks and align the link.
    3. **Configuration**: The devices negotiate the link parameters, such as speed and width, and configure the link accordingly.
    4. **L0 (Active)**: The link is fully trained and operational, allowing normal data transfer.
    5. **Recovery**: If an error is detected, the link may enter a recovery state to attempt to re-establish a stable connection.
    6. **L0s and L1 (Low Power States)**: These states are used for power management when the link is idle.

    ### Summary
    The PCIe link training process is essential for ensuring a reliable and high-performance connection between PCIe devices. It involves detecting the connected devices, negotiating link parameters, performing signal equalization, and transitioning through various states managed by the LTSSM to achieve a stable and fully operational link. This process enables PCIe devices to communicate efficiently, taking full advantage of the available bandwidth and minimizing errors.

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

