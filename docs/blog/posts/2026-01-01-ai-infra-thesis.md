---
date: 2026-01-01
categories:
  - System Design
title: My AI Infra Thesis of 2026
description: My years-long bet of AI infrastructure and how it will shape the major cloud providers.
---

As we enter 2026, AI infrastructure is starting to show the same structural pressures every compute boom eventually does. This post is a snapshot of how I think GPU infrastructure will evolve as the industry exits its growth-at-all-costs phase.

<!-- more -->

## Background

AI infrastructure is entering its "adult" phase. The early AI boom optimized for rapid deployment of supercomputers with as high performance as possible and as minimal abstractions as possible. This has led to huge, single-tenant clusters, bare metal everywhere, over-provisioning of hardware, and weak isolation boundaries. AI researchers consequently need to be bothered with many low-level details of how to design performant training workflows on top of complex and exotic hardware. They need proficiency in many Linux topics just to understand how to interact properly with their software and hardware environments. AI researchers, as anyone who works in the public cloud sector already knows, are generally not good at Linux system administration. They're good at research and building models.

_This phase does not scale economically or operationally._

As all tech booms eventually do, this AI cycle will enter a sort of steady state. This will shift the constraints to:

- Utilization efficiency
- Security between tenants
- Operational consistency
- Developer cognitive load.

Humanity can be characterised as an endless march towards specialization. Asking AI researchers to be good at _both_ AI research _and_ Linux system administration is antithetical to that march. This is the clearest and most lucid point of improvement AI cloud providers can tackle. _Giving researchers container-grade ergonomics with VM-grade tenancy isolation is the goal_. To that end, there are two main competitors I have found in this space:

- [Kata Containers](https://katacontainers.io/)
- [Edera](edera.dev)

These are not the only possible implementations, but they are the clearest expressions today of a broader architectural shift. I will refer to these two products collectively as "Containers in VMs" (CVM).

## Thesis

CVM is the missing middle layer for GPU infrastructure: VM-grade isolation with container-grade ergonomics. As AI infrastructure moves from this growth-at-all-costs phase we are currently in, providers will eventually need stronger security boundaries, higher utilization, and simpler developer workflows. The CVM model enables secure, multi-tenant GPU sharing while preserving the container-based mental model that researchers and ML engineers already rely on. This makes it a foundational technology for democratizing access to high-performance GPUs without forcing users to manage full Linux VMs or bare metal instances.

## Containers are the user interface, VMs are the implementation

Researchers and ML engineers don't want to manage Linux environments. They don't want to tackle with NFS mounts, user directories, kernel drivers, CPU pinning, kernel cmdlines, they just want to run their workloads god dammit! The CVM model flips the burden of infrastructure management. The users now think only in OCI containers while the providers absorb the complexity of VMs and the Linux environment management. Security and isolation between tenants becomes invisible.

## Where CVM Wins

CVM wins when:

- Tenants are untrusted
- NVIDIA Superpod systems must be shared across org boundaries
- Compliance, security, or data isolation matters
- Jobs are long-running (training, fine-tuning, inference)
- Providers want container UX with VM isolation.

## Where CVM Loses

- Single-tenant bare metal is acceptable
- Absolute lowest latency matters more than isolation
- Users already manage full VMs comfortably.

## Why now?

Even as recently as a couple of years ago, VMs would take incredible amounts of time to boot when datacenter-grade NVIDIA GPUs were attached to them. Most of this performance penalty came from DMA (IOMMU) and page table creations. The developments in the Linux kernel with things like IOMMUFD, dma-buf, VFIO improvements, QEMU, Kubernetes CDI specs etc. within the last 2-3 years have dramatically reduced the boottime performance problem to an almost acceptable level. This puts us in a position where VMs can boot fast enough to compete with native runc OCI containers.

Running containers inside VMs is not a new idea, but the technology is improving in such a way that will fundamentally change how AI researchers expect to interact with high performance clusters (in my humble opinion). The days of researchers wrangling full Linux systems are numbered. The visibility of traditional HPC workload schedulers like SGE and Slurm will diminish. I’m betting my career on secure GPU multi-tenancy with container ergonomics becoming the dominant model for AI infrastructure.

## Epilogue

I'm being intentionally vague in this post, almost to a degree I personally find unnatural. Nonetheless, those in the industry who work with Kata Containers or Edera or the many AI-focused neoclouds out there will have an implicit understanding of the problems facing AI researchers today and how current neocloud offerings fall short. I'll continue my research into both Kata and Edera and will be sharing my perspectives on the two projects when the time is right. Stay tuned!
