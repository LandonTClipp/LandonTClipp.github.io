---
title: Databases
icon: octicons/database-16
---

This is just a dumping ground for various database notes I may have.

SQL
----

### Dolthub


https://www.dolthub.com/blog/2021-09-17-database-version-control/

Dolthub is a SQL-compliant (specifically, Postgresql) relational database that tracks changes using version control semantics, not unlike Git. You can branch changes, merge, fork, commit, undo commits, etc just like in Git. It's a pretty neat idea, but from reading the docs, it appears that write operations don't scale well due to the nature of how changes are persisted. However for non-write heavy workloads that could benefit from having comprehensive change history, this is a great solution.

### Views

Most SQL databases have a concept of a "materialized" and "non-materialized" view. A materialized view is a query that is pre-computed and stored in a temporary table. This temporary table can be queried like any normal table, however the results typically don't change until the view is re-computed.

A non-materialized view is simply a query that is performed on another table and presented, usually, as a different schema. Again, the non-materialized view can be queried just like a normal table. The main difference is that the query on the backend tables is run each and every time a query is done on the view.
