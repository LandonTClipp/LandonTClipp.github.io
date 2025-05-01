---
title: Hugepages
icon: material/memory
---

Huge pages are a memory optimization technique whereby you grant your application memory space that uses larger memory page allocation sizes. The typical page size is 4096 bytes, but by enabling hugepages, you can get much larger page sizes. This improves performance in workloads that use large blocks of memory because there will be fewer requests sent to the page cache.

### Allocating Huge Pages

The following examples are stolen from [this](https://rigtorp.se/hugepages/) blog post.

#### Using `mmap`

```C
void *ptr = mmap(NULL, 8 * (1 << 21), PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                 -1, 0))
```

You can also link these mappings to a named file descriptor on the `hugetlbfs` filesystem. Hugepages are drawn from a pool of allocated pages. The size of this pool can be modified.

#### Kernel command-line parameter

https://www.kernel.org/doc/html/latest/admin-guide/mm/hugetlbpage.html

The `hugepages` parameter can be provided to the kernel to reserve a pool of huge pages. This can also be allocated at runtime using the `procfs` or `sysfs` interface.

For example:

```
root@primary:/home/ubuntu# echo 20 > /proc/sys/vm/nr_hugepages
root@primary:/home/ubuntu# cat /proc/sys/vm/nr_hugepages
20
```

!!! note

       Specifying the kernel command-line parameter is the more reliable method of allocating hugepage pools, as memory has not yet become fragmented. It's possible hugepage allocation can fail at runtime due to fragmentation.

You can also specify more exact sizes:

```
root@primary:/home/ubuntu# cat /sys/kernel/mm/hugepages/hugepages-2048kB/free_hugepages
20
root@primary:/home/ubuntu# ls /sys/kernel/mm/hugepages/
hugepages-1048576kB  hugepages-2048kB  hugepages-32768kB  hugepages-64kB
```

### Transparent Huge Pages

Enabling THP allows the kernel to automatically promote regular pages into huge pages.

References:

- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/configuring-huge-pages_monitoring-and-managing-system-status-and-performance
- https://rigtorp.se/hugepages/