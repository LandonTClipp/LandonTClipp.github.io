---
title: Databases
---

## Strategies for Heavy Distributed Writes in SQL

### Master-Slave Replication

You could have multiple replications of your master database that provide various writable locations. However, this still runs into the issue of needing to lock your database when accessing shared data.

### Sharding

You can shard your database such that each node is responsible only for a particular part of the DB, based on the partition key.

### Batched updates

#### KV Store
You could put writes into a distributed key-value store, and have some sort of "flushing" script that periodically sweeps the KV store and persists it to SQL. 

#### Message Queue

Or, you could use message queues and batch the messages in groups of 1000's (for example).

#### Data lake

You could store the SQL updates in a sqlite file (ugh...) and import that directly into the MySQL/PostgreSQL/WhateverSQL. This only works if you can reasonably assume that the updates will not encounter any race conditions (for example, if the updates only consist of _adding_ rows, and not _modifying_ rows).

