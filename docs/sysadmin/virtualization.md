---
title: Virtualization
---


KVM
----

![KVM versus QEMU Diagram](https://f005.backblazeb2.com/file/landons-blog/assets/images/virtualization/qemu-and-kvm-diagram.png)

The Linux kernel provides a technology called the Kernel-based Virtual Machine. KVM makes use of the HVM (hardware virtual machine) extensions to give VMs direct access to host hardware. While KVM can be used by itself to launch virtual machines, it is not uncommon to see QEMU used in conjunction with KVM.


QEMU
-----

The Quick Emulator is a full hardware emulator that allows you to run a VM for any kind of architecture, which is its main advantage over KVM. However, QEMU is much slower as instructions are typically not run natively on the host hardware but are rather dynamically translated. QEMU is also often more difficult to configure than KVM.

QEMU can work directly with KVM to allow acceleration on host platforms that support it. Lack of KVM support means QEMU must fully emulate the hardware which makes it much slower.

