---
title: Databases
---

This is just a dumping ground for various database notes I may have.

Dolthub
-------

https://www.dolthub.com/blog/2021-09-17-database-version-control/

Dolthub is a SQL-compliant (specifically, Postgresql) relational database that tracks changes using version control semantics, not unlike Git. You can branch changes, merge, fork, commit, undo commits, etc just like in Git. It's a pretty neat idea, but from reading the docs, it appears that write operations don't scale well due to the nature of how changes are persisted. However for non-write heavy workloads that could benefit from having comprehensive change history, this is a great solution.
