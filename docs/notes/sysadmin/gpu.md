---
icon: simple/nvidia
---

# GPU


## Architectures

| Name | Launch Date | Description |
|------|-------------|-------------|
| [Blackwell](https://en.wikipedia.org/wiki/Blackwell_(microarchitecture)) | March 18, 2024 | |
| [Grace Hopper](https://en.wikipedia.org/wiki/Hopper_(microarchitecture)#Grace_Hopper) | Unknown | Combines the Grace-based 72-core CPU and Hopper-based H200 GPU on a single module. CPU+GPU are connected via NVLink |
| [Hopper](https://en.wikipedia.org/wiki/Hopper_(microarchitecture)) | Sept 20, 2022 | |



## Models

| Type | Release Date | Description |
|------|--------------|-------------|
| GH200 | Unknown | Combines Grace and Hopper architectures using NVLink-C2C |

## Driver

### `/proc/driver/nvidia`

You can use this filesystem to query device information directly using the NVIDIA driver.

```
$ cat /proc/driver/nvidia/gpus/0000\:62\:00.0/information
Model:           NVIDIA H100 80GB HBM3
IRQ:             10
GPU UUID:        GPU-9221d5d1-b60b-a624-bf14-0a1819cfda3b
Video BIOS:      96.00.99.00.01
Bus Type:        PCIe
DMA Size:        52 bits
DMA Mask:        0xfffffffffffff
Bus Location:    0000:62:00.0
Device Minor:    5
GPU Firmware:    535.129.03
GPU Excluded:    No
```

### `/proc/sys/bus/pci`

You can use this to query various PCI-related things.

#### `config`

The config file contains the PCIe config space

```
$ hexdump /sys/bus/pci/devices/0000\:62\:00.0/config
0000000 10de 2330 0547 0010 00a1 0302 0000 0000
0000010 000c 0200 38c0 0000 000c 0000 38a0 0000
0000020 000c 0000 38c0 0000 0000 0000 10de 16c1
0000030 0000 0000 0040 0000 0000 0000 010a 0000
0000040 4801 0013 0008 0000 6005 0188 0000 0000
0000050 0000 0000 0000 0000 0000 0000 0000 0000
```

The first two bytes contain the vendor ID. You'll see this should match with the `vendor` file.

#### `vendor`

```
$ cat /sys/bus/pci/devices/0000\:62\:00.0/vendor
0x10de
```

#### `resource`

https://docs.kernel.org/PCI/sysfs-pci.html

This is a file that contains PCI host resource addresses.

#### `resource0`

`resourceN` are files that can be mmap-ed to in order to read device memory.

```python
    resource_path = Path("/sys/bus/pci/devices/0000:62:00.0/resource0")

    with resource_path.open(mode="rb") as f:
        mem = mmap.mmap(f.fileno(), 16 * 1024 * 1024, access=mmap.ACCESS_READ)
        boot0_region = struct.unpack("<L", mem[:4])[0]
        ptherm_region = struct.unpack("<L", mem[0x020400:0x020404])[0]
```

The exact memory locations depend on the device manufacturer. Consult the manufacturer for more information on what memory locations mean.

## nvswitch

nvswitch is an NVLink switch that is used to enable cross-GPU communication on NVLink. You can view the available nvswitches on the PCI bus:

```
$ lspci | grep NVIDIA
05:00.0 Bridge: NVIDIA Corporation Device 22a3 (rev a1)
06:00.0 Bridge: NVIDIA Corporation Device 22a3 (rev a1)
07:00.0 Bridge: NVIDIA Corporation Device 22a3 (rev a1)
08:00.0 Bridge: NVIDIA Corporation Device 22a3 (rev a1)
```

Viewing verbose information of these devices:

```
$ lspci -vvv -s '05:00.0'
05:00.0 Bridge: NVIDIA Corporation Device 22a3 (rev a1)
        Subsystem: NVIDIA Corporation Device 1796
        Physical Slot: 1
        Control: I/O- Mem+ BusMaster- SpecCycle- MemWINV- VGASnoop- ParErr+ Stepping- SERR+ FastB2B- DisINTx-
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Interrupt: pin A routed to IRQ 16
        NUMA node: 0
        IOMMU group: 5
[...]
```

!!! tip "Unhealthy nvswitch"

    Often, an nvswitch can be detected as being unhealthy if the lspci output shows `(rev ff)`. This indicates corruption on the device.
