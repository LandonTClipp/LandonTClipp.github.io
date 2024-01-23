---
title: Computer Science
---

This page describes various algorithmic techniques and terms related to computer science.

[Memoization](https://en.wikipedia.org/wiki/Memoization#:~:text=In%20computing%2C%20memoization%20or%20memoisation,the%20same%20inputs%20occur%20again.)
---------------

Memoization refers to caching the result of an expensive function call given a set of inputs.

[Optimal Substructure](https://en.wikipedia.org/wiki/Optimal_substructure)
---------------------

Optimal Substructure is a term used to describe problems where the optimal solution can be found by finding the optimal solution to its subproblems. Greedy algorithms can be used to solve such problems if it can be proven by induction that the solution can be optimal at each step.

Trees
------

### [Trie](https://en.wikipedia.org/wiki/Trie)

![A trie diagram](https://upload.wikimedia.org/wikipedia/commons/b/be/Trie_example.svg)

A Prefix Tree, or a trie, is a type of search tree used for locating specific keys from within a set. Each node of the trie represents a common prefix that all of its descendents contain. The name "trie" (pronounced try) comes from the word re**trie**val.

Each node stores a single character, and it has at most 26 children, one for each letter of the alphabet.

Backpressure
------------

Not necessarily a computer science topic, but it describes a scenario where a consumer of some resource (whether it be a queue, or a proxy, etc) cannot consume faster than things are pushing to this resource. This situation will eventually cause the resource to become overloaded as it queues the requests in-memory (or disk) for the consumer.

Data Locality
--------------

Data locality refers to how data is laid out in memory or disk. In general, read requests for a particular data structure will read things sequentially, meaning that a request for an element at index `n`, for example, will often be followed up by requests at `n+1`. In most memory and storage systems, data retrieval for `n` will cause large blocks of data to be pulled from the backing storage and cached in memory, so we generally want `n+1` to be _local_ to `n`. This data locality trait will allow us to take advantage of these caching systems and improve performance.

In computer memory, the CPU has three layers of cache: L1 (typically the smallest), L2 (larger), and L3 (the largest, shared by all cores). Main memory access is extremely slow while each layer of the CPU cache is progressively faster than the higher layer. This is why it's preferrable to take full advantage of each caching layer, instead of missing back to main memory on every access.
