---
title: Golang
---

Golang
=======

Garbage Collector
------------------

stub

Channels
--------

stub

Maps
-----

stub

OOP (in Go)
-----------

stub

Slices
-------

stub

Goroutines
-----------

stub

Functional Options
-------------------

stub

Tools as Dependencies
----------------------

stub

TODO: Show the `tools/` sub-module approach to setting tool dependencies.

Types
-----

### rune

A rune is an alias for `int32`. It represents a code point in the Unicode standard. The elements of a string (which are just singular bytes) can be converted to a rune through a simple `#!go rune(elem)` type conversion. It's important to know that some characters in the Unicode specification can be represented by different bytes. For example, à can be represented in two ways: either it's explicit code point U+00E0, or through a sequence of two separate code points of U+0061 (which is the lowercase `a`) and U+0300 (which is the "combining" grave access point, which can be applied to any character). This form of character representation would require two elements in a string type in Go because it requires two bytes, but can be easily represented by a single rune.
