---
date: 2023-07-13
categories:
  - programming
  - golang
title: Preferred Golang Readings
description: A list of articles exploring Golang
---

Preferred Golang Readings
========================

This post documents some exemplary blog posts and articles exploring interesting topics in Go performance, theory, and general news.

<!-- more -->

Faster software through register based calling
-----------------------------------------------

https://menno.io/posts/golang-register-calling/

My coworker Menno wrote about register-based calling in Go 1.17. He uses this example program:

```go
package main

import "fmt"

func add(i, j int) int {
    return i + j
}

func main() {
    z := add(22, 33)
    fmt.Println(z)
}
```

When adding two integers in go, the compiler will make something like this:

```
    movq   $0x16,(%rsp)
    movq   $0x21,0x8(%rsp)
    call   4976e0 <main.add>
    mov    0x10(%rsp),%rax
    mov    %rax,0x30(%rsp)
```

It moves the literal values `$0x16` and `$0x21` onto the memory location pointed to in `(%rsp)` and `(%rsp)+0x8`. The results are then retrieved off of the stack and are put into `%rax`.

In 1.18, Go prefers passing arguments through registers:

```
    mov    $0x16,%eax
    mov    $0x21,%ebx
    xchg   %ax,%ax
    call   47e260 <main.add>
```

Something else Menno noted was that Go doesn't use the `push` instruction and instead uses `mov` to push values onto the stack. I was initially confused by this as well, but he notes that `mov` is generally faster: https://agner.org/optimize/instruction_tables.pdf



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

