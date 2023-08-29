---
date: 2023-08-12
categories:
  - programming
  - golang
title: Explicit Error Return Values Are Bad
description: Why languages that have errors as return codes are worse than exception-based languages
---

Over my years as a professional programmer, I've dealt with a number of languages that have different philosophies on how errors should be handled. Many debates have raged over what error model languages should adopt, so in this post, I will be showing you why the errors-as-return-values (EARV) model of error handling is inferior to errors-as-exceptions (EAE) model, the pitfalls of ERV, and the reasons why EAE leads to more safe systems.

<!-- more -->

Some Principles for Good Language Design
--------------

1. Boilerplate is bad
2. Errors should cause a program to crash by default
3. Errors should be difficult to ignore
4. Errors should only be thrown away through explicit, unambiguous intention by the programmer
5. 

Arguments Against EAE
-----------------

1. Exception flows are hard to reason about
2. I don't know where an exception will be handled
3. 

Arguments For EARV
------------------

1. The flow is obvious
2. It makes you think hard about the errors
3. 


In C, user-level errors aren't part of the language spec, but are typically implemented as integer return codes. In Bash, error codes are in the special `$?` variable that can be checked after every command. The glut of C++ dialects let you choose between errors-as-return-codes (ERC) or exception-based errors. In Golang, you must return an `error` type from your functions which could fail.