---
title: Hardware
---

Hard Drives
----------

### SSD
Solid State Drives

#### SATA

A SATA (Serial AT Attachment) is a computer bus interface that many of the early SSD disks used. This is commonly used in desktop hardware, although it's starting to be phased out for NVMe.

Typical read speeds are 550MB/s

#### NVMe

Non-Volatile Memory Express is a more modern SSD that has much higher speeds than SATA. It typically comes in the M.2 form factor, but also come in U.2 and PCIe cards.

Typical read speeds are up to 3,500 MB/s for PCIe Gen 3, 7,500MB/s for PCIe Gen 4.

### HDD

Hard Disk Drives are drives which use magnetic spinning disks to store data. They are used for data which does not require high throughput, as disk head seek times can be quite high. 

Typical read speeds are between 80MB/s and 160MB/s.

### RAID

RAID, or Redundant Array of Inexpensive Disks, is a data storage virutalization technology used for exposing multiple independent disks as a single hard drive to the operating system. Most Dell servers come with hardware RAID support natively, but you can also utilize a software-based implementation. RAID arrays offer data redundancy and increased throughput, depending on the RAID level.

| Level | Meaning |
|-------|---------|
| RAID 0 | Block-level striping, but no mirroring or parity. The contents of a single file can be distributed amongst multiple drives. The failure of any one disk can cause corruption in many files. The total capacity is the sum of all the drives in the array. |
| RAID 1 | Provides data mirroring, without parity or striping. Data is written identically to two or more drives. Sustained read throughput approaches the sustained throughput of all the drives combined. Write throughput is usually slower than RAID 0 because data has to be duplicated (the slowest drive limits performance). |
| RAID 2 | Provides bit-level striping with dedicated Hamming-code parity. Each sequential bit is on a different drive. Hamming-code parity is calculated using the parity code stored on at least one drive. This level is usually not used by any commercial system. |
| RAID 3 | Provides byte-level striping with dedicated parity. Each sequential byte is stored on different drives. RAID 3 is not commonly used. |
| RAID 4 | Provides block-level striping with dedicated parity. |