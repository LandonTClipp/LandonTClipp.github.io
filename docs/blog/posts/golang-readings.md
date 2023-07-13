---
date: 2023-07-13
categories:
  - programming
  - golang
title: Preferred Goland Readings
description: A list of articles exploring Golang
---

Generics Degrade Performance
---------------------------

https://planetscale.com/blog/generics-can-make-your-go-code-slower

Accessing the function pointer if you call a generic function with a pointer to an interface requires three dereferences:

```
MOVQ ""..dict+48(SP), CX # load the virtual table from the stack into CX
0094  MOVQ 64(CX), CX.    # dereference the dictionary to get the *itab 
0098  MOVQ 24(CX), CX.    # Offset by 24 bytes to load the function pointer within *itab
```

Another interesting point, passing an interface to a generic function where the interface is a superset of the type constraint is what the author calls a "footcannon," because the compiler does this nonsense:

```
00b6  LEAQ type.io.ByteWriter(SB), AX
00bd  MOVQ ""..autotmp_8+40(SP), BX
00c2  CALL runtime.assertI2I(SB)
00c7  MOVQ 24(AX), CX
00cb  MOVQ "".buf+80(SP), AX
00d0  MOVL $92, BX
00d5  CALL CX
```

The compiler is asserting that the interface you passed does indeed implement the interface in the type constraint (runtimeassertI2I), and it grabs the *itab of the interface in the type constraint. This tanks performance.

