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

Communication Protocols
-----------------------

### [gRPC](https://grpc.io/)

![gRPC Diagram](https://grpc.io/img/landing-2.svg)

Used as a remote procedure call. Uses Protobuf to send and receive serialized structured data. Messages in protobuf are defined in a [language-agnostic DSL](https://protobuf.dev/programming-guides/proto3/).

### [Websocket](https://en.wikipedia.org/wiki/WebSocket)

![Websocket diagram](https://assets-global.website-files.com/5ff66329429d880392f6cba2/63fe488452cc63cf1cb0ae45_148.2.png)

Websocket is a web communication protocol that allows you to asynchronously send and receive messages over the same connection. It starts its life as an HTTP connection, and then is upgraded to a websocket session. This protocol is the answer to common HTTP polling techniques that are used to periodically check for updates from the server. Some examples of where this protocol is used:

1. Real-time web applications where updates need to be continuously streamed.
2. Chat applications where messages arrive asynchronously.
3. Gaming applications where updates are fairly continual.

Many of these kinds of data streaming problems were commonly solved through HTTP polling, however this solution is inefficient due to the constant HTTP/TCP handshakes that would need to occur even if no data was available.

Websocket is generally served through ports 80 or 443 (HTTP/HTTPS), which allows it to pass through many types of firewalls. 

Polling Mechanisms
------------------

### Short polling

A client would periodically poll the server for new messages. This would work but it's inefficient because it requires opening and closing many connections.

### Long polling

Same as short polling, but we can keep the connection open until we receive new messages. The disadvantages are:
- The sender and receiver might not be on the same server.
- The server has no good way to tell if a client is disconnected.
- It's inefficient because long polling will still make periodic connection requests even if there are no messages being sent.

### Websocket

Websocket is a protocol that allows you to asynchronously send and receive messages over the same connection. 

Service Discovery
-----------------

When you have a pool of services, for example a pool of websocket servers that provide clients with messages in a chat system, you often need to provide the client with a list of DNS hostnames they could connect to. This process is called "Server-side Service Discovery" and there are many off-the-shelf solutions:

### [etcd](https://etcd.io/) :simple-etcd:

This is a distributed key-value store that can be used for service discovery (among many other things). It uses an eventually-consistent model through the gossip-based protocol. It does not have many built-in features for service discovery as it focuses more on the key-value store.

### [consul](https://www.consul.io/) :simple-consul:

consul is a Service Mesh that provides a rich suite of metrics and monitoring capabilities. It allows you to define health checks on your services. It also comes with native support for cross-datacenter replication, while etcd does not.

consul also gives you distributed key-value stores

### [Apache Zookeeper](https://en.wikipedia.org/wiki/Apache_ZooKeeper)

This is another distributed key/value store that also provides service discovery.

Key-Value Stores
---------------

- redis
- cassandra

Redis is often chosen over etcd for key-value stores because etcd requires consensus amongst its nodes before committing changes, which slows performance. The benefit of etcd is that it provides a lot of consistency guarantees. The [docs](https://etcd.io/docs/v3.3/learning/api_guarantees/#:~:text=etcd%20is%20a%20consistent%20and,API%20guarantees%20made%20by%20etcd.) claim it follows [Sequential Consistency](https://en.wikipedia.org/wiki/Consistency_model#Sequential_consistency), which means that while a write does not have to be seen instantaneously, the _sequence_ in which writes to a piece of data are seen is identical across all processors.

### Cassandra :simple-apachecassandra:

Cassandra is often compared against Redis. For a good comparison, look [here](https://www.knowledgenile.com/blogs/cassandra-vs-redis). Many of the notes here are drawn from that blog.

This is used by some large chat platforms like Discord to store chat history. Some aspects of Cassandra:

- Uses wide-column store as a database model, which makes it easy to store huge datasets. It acts as a two-dimensional key/val store.
- Focuses on stability, but it's slower than other platforms like Redis. Redis becomes much slower if you store huge datasets, and thus it's more suited for highly variable datasets.
- Cassandra focuses on consistency and partition tolerance, while redis focus on availability and partition tolerance.
- Schema-free.
- Uses Java, uses thrift protocol for API.
- More useful when you have distributed, linearly scalable, write-oriented, and democratic P2P database structure.

### redis :simple-redis:

Meant specifically as an in-memory key-value data store. Most commonly used for distributed key-val.


