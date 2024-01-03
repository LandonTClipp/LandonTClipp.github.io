---
title: CVMFS
---

https://cvmfs.readthedocs.io/en/stable/

CVMFS stands for the CERN VM Filesystem. While it was indeed made at CERN, it is not inteded just for VMs. This is just a historical vestige of what it was originally designed for.

CVMFS is a semi-POSIX-compliant filesystem that utilizes a Merkle Tree for its metadata storage. The nodes within the Merkle Tree reference file data by content hash. This filesystem is designed to handle large amounts of small files, and more specifically, for the software distribution use case.

CVMFS communicates through HTTP both for its metadata and data. This makes it easily cacheable using off-the-shelf caching solutions, such as Varnish. It is designed to be eventually consistent, highly available, and partition tolerant.

I have significant experience with CVMFS in designing software distribution CDNs, so please feel free to ask me about it!
