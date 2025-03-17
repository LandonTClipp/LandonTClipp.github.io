---
title: Infiniband
icon: material/switch
---

Bandwidth Test
--------------

On one node, start a listener:

```
ib_write_bw -d mlx5_1 --report_gbits -s 8388608 -F -f 0 --duration 30 -q 4
```

On another:

```
ib_write_bw -d mlx5_1 [listener_ip] --report_gbits -s 8388608 -F -f 0 --duration 30 -q 4
```

The listener IP does not need to be an IPoIB address. It's a connection that is only used to
negotiate the IB connection.

Pkey
----

A pkey, or Partition Key, is analogous to a VLAN tag. It allows you to partition your network amongst multiple tenancies such that HCAs within a pkey can only talk to other HCAs in that pkey.

Virtualization
---------------

Infiniband cards can take advantage of SR-IOV just like most other PCIe devices.

https://docs.nvidia.com/networking/display/mlnxofedv522230/single+root+io+virtualization+(sr-iov)