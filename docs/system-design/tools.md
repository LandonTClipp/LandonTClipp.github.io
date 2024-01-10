---
title: Tools
---

This page shows various methods, techniques, algorithms, and data structures that may be useful in your system design career.

[Bloom Filters](https://en.wikipedia.org/wiki/Bloom_filter)
-------------

![Bloom Filter diagram](https://upload.wikimedia.org/wikipedia/commons/a/ac/Bloom_filter.svg)

A bloom filter is a probabilistic data structure that tells you either:

1. A particular piece of data is _possibly_ in the set
2. A particular piece of data is _definitely not_ in the set.

It is highly memory efficient (as most filters can reside totally in memory) and easy to implement in a distributed environment. The way it works starts with an array of bit-length N. Then:

1. When you add an element to the set, you take its hash then `OR` the hash's bitwise value with the bloom filter. This adds the hash's bits to the array.
2. To check of an element possibly exists in the set, take its hash and check if the 1's bits in the bloom filter match the 1's bits in the hash
3. If the 1's match, then the element possibly already exists. If they don't, then it's a certainty the element was never added.

The reason why you can only determine "possibly exists in set" is because different hashes can mark the same bit as 1. The probability of these false positives is dependent on the size of the filter and the number of elements added to it. This can be seen in the graph below:

![Bloom Filter false positives probability graph](https://upload.wikimedia.org/wikipedia/commons/e/ef/Bloom_filter_fp_probability.svg)

[Consistent Hashing](https://en.wikipedia.org/wiki/Consistent_hashing)
--------------------

Consistent Hashing is a load balancing technique that allows you to add and remove nodes in a pool without causing massive redistributions in the client/server mappings (called the Rehashing Problem).

<div class="grid cards" markdown>

- ![Hash ring 1](https://sasgidotxvcxfexkslru.supabase.co/storage/v1/object/public/assets/consistent_hashing/consistent_hash_ring_01.png)
- ![Hash ring 2](https://sasgidotxvcxfexkslru.supabase.co/storage/v1/object/public/assets/consistent_hashing/consistent_hash_ring_02.png)

</div>

The general idea is that both servers and users live within the same hash space. The servers, more often than not, will have a hash key that depends on the name. The users will have a hash that depends on their name, but also possibly their location.

When a user is hashed, we find the closest adjacent server in a counter-clockwise manner (it could also be clockwise, it doesn't matter). When a new server gets added, there is only a small probability that any particular user needs to get re-mapped to a new server. Or in other words, the chance of a new server being blaced _between_ a user and that user's prior server is quite low.

In the case that does happen, a re-mapping will indeed have to occur, but the overall probability is low enough that the cache misses should be minimal.

!!! note "Virtual Nodes for Smoothing"
    
    ![Consistent hash ring with virtual nodes](https://sasgidotxvcxfexkslru.supabase.co/storage/v1/object/public/assets/consistent_hashing/consistent_hash_ring_virtual_nodes.png){ align=left width="500" } It's very likely for nodes in a hash ring to become too clumped together, which would cause uneven distribution (this is called the Hotspot Key Problem). You can smooth this out by using virtual nodes: for every real node, we also add some number of virtual nodes that maps back to the real node. This effectively causes the hash ring to become more uniformly distributed as it causes the standard deviation (in terms of empty space in the ring) to be smaller.

You can see in a consistent hashing scheme, adding a node to the ring will cause only _some_ of the clients to be re-mapped.

CAP Theorem
-----------

CAP stands for Consistency, Availability, and Partition Tolerance. The theorem states that distributed systems can exhibit at most two of these traits, which implies that a tradeoff must be made. In most systems, partition intolerance is not acceptable as networks and systems often fail. It's a bit of an oxymoron as well to state that a system is consistent and available, but not partition tolerant. So in reality, the tradeoff is usually between consistency and availability.
