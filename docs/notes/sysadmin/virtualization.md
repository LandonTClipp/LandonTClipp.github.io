---
title: Virtualization
icon: simple/virtualbox
---

Hypervisor
----------

![Hypervisor Types](https://f005.backblazeb2.com/file/landons-blog/assets/images/virtualization/hypervisor-types.jpg)

There are two types of hypervisors, type 1 and type 2. Type 1 hypervisors are services that have direct access to hardware. They do not rely on the host OS to execute instructions.

Type 2 hypervisors sit on top of the host OS which introduces a massive latency penalty.

KVM
----

![KVM versus QEMU Diagram](https://f005.backblazeb2.com/file/landons-blog/assets/images/virtualization/qemu-and-kvm-diagram.png)

The Linux kernel provides a technology called the Kernel-based Virtual Machine. KVM makes use of the HVM (hardware virtual machine) extensions to give VMs direct access to host hardware. While KVM can be used by itself to launch virtual machines, it is not uncommon to see QEMU used in conjunction with KVM.


QEMU
-----

The Quick Emulator is a full hardware emulator that allows you to run a VM for any kind of architecture, which is its main advantage over KVM. However, QEMU is much slower as instructions are typically not run natively on the host hardware but are rather dynamically translated. QEMU is also often more difficult to configure than KVM.

QEMU can work directly with KVM to allow acceleration on host platforms that support it. Lack of KVM support means QEMU must fully emulate the hardware which makes it much slower.

### Socket Types

#### AF_VSOCK

AF_VSOCK is a bidirectional socket used for secure communication between a hypervisor and a VM. QEMU can be configured to create this channel through the domain XML: https://libvirt.org/formatdomain.html#vsock

!!! tip
    If the `auto` option is used, you can determine what CID libvirt gave to the socket by using `virsh dumpxml [domain]`. It will show you the assigned CID (called an `address`) in the domain XML.
    You can then use this address to open the communication channel as shown below.

An example of a hypervisor and VM communicating:

```
root@hypervisor:/home/landon# socat - VSOCK-CONNECT:3:1234
host: hello
vm: well hi der
```

```
root@virtual-machine:/home/landon# socat VSOCK-LISTEN:1234,fork -
host: hello
vm: well hi der
```

[Firecracker](https://firecracker-microvm.github.io/)
-----------

![Firecracker Diagram](https://firecracker-microvm.github.io/img/diagram-desktop@3x.png)

Firecracker is an alternative to QEMU that is built specifically for microVMs. It was created by AWS for the Lambda service.

[SR-IOV](https://en.wikipedia.org/wiki/Single-root_input/output_virtualization)
-------

![SR-IOV diagram](https://access.redhat.com/webassets/avalon/d/Red_Hat_Enterprise_Linux-7-Virtualization_Deployment_and_Administration_Guide-en-US/images/fac9cf14c66b5feb845c5039c89d88ab/SR-IOV_implementation.png)

SR-IOV, or Single-Root I/O Virtualization, is a PCI-SIG standard that allows the isolation of PCIe resources for performance and manageability reasons. SR-IOV allows a device, like a network adapter, to separate access to its resources among various PCIe hardware functions. This is achieved through the introduction of two PCIe functions: Physical Functions (PFs) and Virtual Functions (VFs).

SR-IOV enables a Single Root Function (such as an ethernet port) to appear as multiple, separate physical devices. A physical device with SR-IOV capabilities can be configured to appear in the PCI configuration space as multiple functions.

Resources:

- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-pci_devices-pci_passthrough

### Physical and Virtual Functions

SR-IOV uses physical and virtual functions to control or configure PCIe devices. Physical functions have the ability to move data in and out of the device while virtual functions are lightweight PCIe functions that support data flowing but also have a restricted set of configuration resources. The virtual or physical functions available to the hypervisor or guest operating system depend on the PCIe device.

Physical Functions (PFs) are full PCIe devices that include the SR-IOV capabilities. Physical Functions are discovered, managed, and configured as normal PCI devices. Physical Functions configure and manage the SR-IOV functionality by assigning Virtual Functions.

Virtual Functions (VFs) are simple PCIe functions that only process I/O. Each Virtual Function is derived from a Physical Function. The number of Virtual Functions a device may have is limited by the device hardware. A single Ethernet port, the Physical Device, may map to many Virtual Functions that can be shared to virtual machines.


### Configuration

SR-IOV is enabled through the system BIOS.

https://docs.nvidia.com/networking/display/mlnxofedv582030lts/single+root+io+virtualization+(sr-iov)#src-2396585884_safe-id-U2luZ2xlUm9vdElPVmlydHVhbGl6YXRpb24oU1JJT1YpLXNldHRpbmd1cHNyLWlvdg

