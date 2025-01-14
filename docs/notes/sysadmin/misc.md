---
icon: octicons/info-16
---

# Misc


[NUMA](https://en.wikipedia.org/wiki/Non-uniform_memory_access)
---------

Most modern datacenter servers have multiple CPU sockets. Each socket is generally physically close to a portion of the main memory. Each socket is capable of accessing the memory of another NUMA node, however this incurs performance penalties as it has to traverse the memory bus.


[dmidecode](https://en.wikipedia.org/wiki/Dmidecode)
----------

Use `dmidecode` to read verbose information on all hardware being used on the computer. This tool decodes the [SMBIOS](https://en.wikipedia.org/wiki/System_Management_BIOS) table in a human-readable format.

The acronym `DMI` refers to the Desktop Management Interface which is a closely-related standard to SMBIOS that `dmidecode` was originally written for.
