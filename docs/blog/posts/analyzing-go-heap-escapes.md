---
date: 2023-07-15
categories:
  - programming
  - golang
title: Analyzing Go Heap Escapes
description: Analyzing how Go decides what escapes a heap, and how to visualize it in VSCode Codelens.
draft: true
---

Analyzing Go Heap Escapes
=========================

In this blog post, we discover how you can analyze what variables the Go compiler decides should escape to the heap, a [common source of performance problems](https://tip.golang.org/doc/gc-guide#Eliminating_heap_allocations) in Golang. We'll also show how you can configure the gopls language server in VSCode to give you a Codelens view into your escaped variables.

<!-- more -->

What is a Heap?
---------------

The working memory of most modern programs is divided into two main categories: the _stack_, which contains short-lived memory whose lifetime is intrinsically tied to the lifecycle of the stack of function calls, and the _heap_, which contains long-lived memory whose lifetime transcends your stack of function calls. The Go compiler has to make a decision on where a particular piece of data should reside, by running what's called an Escape Analysis Algorithm. If the analysis decides that an object can be referenced outside of the lexical scope which created it, it will allocate it on the heap.

Why is the Heap a Problem?
-------------------------

Garbage-collected languages like Go have to periodically sweep the tree of object references allocated on the heep to determine if the reference is reachable (meaning some part of the code might still potentially access it) or if the reference is orphaned. If it's orphaned, it's impossible for the code to ever use it, so we should free that memory. This process is highly memory-intensive and slows execution of the application. The garbage collector is a necessary evil due to the fact that Go does not require the programmer to manually free memory.

Heap Escape Analysis
--------------------

### Returning pointers to local objects

We can run standard go CLI tools to generate an analysis of our code here:

```go linenums="1"
package main

import "fmt"

func foobar() *string {
	foobar := "foobar"
	return &foobar
}

func main() {
	r := foobar()
	fmt.Print(*r)
}

```

We call `go build` with the following garbage collector flags to tell it to generate debug info on various decisions it's made, and output the results into a directory called `out`:

```bash
$ go build -gcflags='-m=3 -json 0,file://out' . |& grep escape
# go-heap-escapes
./main.go:6:2: foobar escapes to heap:
./main.go:12:12: *r escapes to heap:
./main.go:12:11: ... argument does not escape
./main.go:12:12: *r escapes to heap
```

It outputs `main.json` that tells us more detailed information:

```json title="out.json"
{"version":0,"package":"main","goos":"darwin","goarch":"arm64","gc_version":"go1.20.6","file":"/Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go"}
{"range":{"start":{"line":5,"character":6},"end":{"line":5,"character":6}},"severity":3,"code":"canInlineFunction","source":"go compiler","message":"cost: 8"}
{"range":{"start":{"line":6,"character":2},"end":{"line":6,"character":2}},"severity":3,"code":"escape","source":"go compiler","message":"foobar escapes to heap","relatedInformation":[{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":7,"character":9},"end":{"line":7,"character":9}}},"message":"escflow:    flow: ~r0 = \u0026foobar:"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":7,"character":9},"end":{"line":7,"character":9}}},"message":"escflow:      from \u0026foobar (address-of)"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":7,"character":2},"end":{"line":7,"character":2}}},"message":"escflow:      from return \u0026foobar (return)"}]}
{"range":{"start":{"line":10,"character":6},"end":{"line":10,"character":6}},"severity":3,"code":"cannotInlineFunction","source":"go compiler","message":"function too complex: cost 91 exceeds budget 80"}
{"range":{"start":{"line":12,"character":12},"end":{"line":12,"character":12}},"severity":3,"code":"escape","source":"go compiler","message":"*r escapes to heap","relatedInformation":[{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":12},"end":{"line":12,"character":12}}},"message":"escflow:    flow: {storage for ... argument} = \u0026{storage for *r}:"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":12},"end":{"line":12,"character":12}}},"message":"escflow:      from *r (spill)"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:      from ... argument (slice-literal-element)"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:    flow: fmt.a = \u0026{storage for ... argument}:"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:      from ... argument (spill)"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:      from fmt.a := ... argument (assign-pair)"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:    flow: {heap} = *fmt.a:"},{"location":{"uri":"file:///opt/homebrew/Cellar/go/1.20.6/libexec/src/fmt/print.go","range":{"start":{"line":272,"character":15},"end":{"line":272,"character":15}}},"message":"inlineLoc"},{"location":{"uri":"file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go","range":{"start":{"line":12,"character":11},"end":{"line":12,"character":11}}},"message":"escflow:      from fmt.Fprint(os.Stdout, fmt.a...) (call parameter)"},{"location":{"uri":"file:///opt/homebrew/Cellar/go/1.20.6/libexec/src/fmt/print.go","range":{"start":{"line":272,"character":15},"end":{"line":272,"character":15}}},"message":"inlineLoc"}]}
{"range":{"start":{"line":12,"character":12},"end":{"line":12,"character":12}},"severity":3,"code":"escape","source":"go compiler","message":""}
```

If we format the third entry, we see something very interesting:

```json title="main.json"
{
  "range": {
    "start": {
      "line": 6,
      "character": 2
    },
    "end": {
      "line": 6,
      "character": 2
    }
  },
  "severity": 3,
  "code": "escape",
  "source": "go compiler",
  "message": "foobar escapes to heap",
  "relatedInformation": [
    {
      "location": {
        "uri": "file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go",
        "range": {
          "start": {
            "line": 7,
            "character": 9
          },
          "end": {
            "line": 7,
            "character": 9
          }
        }
      },
      "message": "escflow:      from &foobar (address-of)"
    },
    {
      "location": {
        "uri": "file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go",
        "range": {
          "start": {
            "line": 7,
            "character": 2
          },
          "end": {
            "line": 7,
            "character": 2
          }
        }
      },
      "message": "escflow:      from return &foobar (return)"
    }
  ]
}
```

This is telling us very clearly that `#!json "message": "foobar escapes to heap",`. It's pretty obvious why, let's take a closer look.

```go linenums="1"
func foobar() *string {
	foobar := "foobar"
	return &foobar
}
```

This function instantiates a string named `foobar` that has the value `"foobar"`. We return a pointer to `foobar` which then means that the string initially allocated on the stack of `func foobar() *string` can now be referenced by functions outside of this lexical scope. So the variable can't remain on the stack; it _has_ to escape to the heap.

The JSON even tells us exactly why it escapes and what sequences of events had to happen for it to escape. By following the `escflow` messages, we can see the three necessary events were:

```json
"message": "escflow:      from &foobar (address-of)"
"message": "escflow:      from return &foobar (return)"
```

It escapes because we:

1. We took the address of `foobar`
2. We returned that address

### Escapes of `fmt.Print`

We also see that there's another escape on line 12 in the `fmt.Print`. 

```json
{
  "range": {
    "start": {
      "line": 12,
      "character": 12
    },
    "end": {
      "line": 12,
      "character": 12
    }
  },
  "severity": 3,
  "code": "escape",
  "source": "go compiler",
  "message": "*r escapes to heap",
// ...
```

What's going on here? `fmt.Print`'s argument is a [variadic argument of type `any`](https://pkg.go.dev/fmt#Print) which basically means we're passing `#!go []any{"foobar"}`, where the string `"foobar"` is the object that we've already shown was allocated to the heap (but this fact is irrelevant to the question of why the slice itself escaped to the heap). `any` is just an alias for `inteface{}` so we're passing a slice of interfaces. Why is this a problem? Let's try with our own simple `print` implementation that instead uses `...string`:

```go linenums="1"
package main

import (
	"io"
	"os"
)

func print(w io.Writer, s ...string) {
	var buf []byte
	for _, arg := range s {
		for _, elem := range arg {
			buf = append(buf, byte(elem))
		}
	}
	w.Write(buf)
}

func main() {
	foo := "foo"
	print(os.Stdout, foo)
}
```

Does this escape? The analysis says no!

```bash
$ go build -gcflags='-m=3 -json 0,file://out' . |& grep escape
# go-heap-escapes
./main.go:8:25: s does not escape
./main.go:19:7: ... argument does not escape
```

This brings us to the next point:

### Interfaces can cause heap escapes

This is not a new discovery. It has been known about [for a long time](http://npat-efault.github.io/programming/2016/10/10/escape-analysis-and-interfaces.html) by [multiple different bloggers](https://www.ardanlabs.com/blog/2023/02/interfaces-101-heap-escape.html). It turns out that the Go compiler is incapable of knowing at compile-time whether the underlying type in an interface is a value type or a pointer. As we already know, the compiler has to assume that a pointer's dereferenced value can outlive the life of its lexical scope, so it has to make the conservative assumption that the slice can contain pointers.


We can see this is true even in the simple case where the argument is a bare interface:

```go linenums="1"
package main

import (
	"bytes"
	"io"
)

func print(w io.Writer, s string) {
	asBytes := []byte(s)
	w.Write(asBytes)
}

func main() {
	buf := bytes.Buffer{}
	print(&buf, "foobar")
}
```
<div class="result">
```title=""
./main.go:9:20: ([]byte)(s) escapes to heap:
./main.go:9:20:   flow: asBytes = &{storage for ([]byte)(s)}:
./main.go:9:20:     from ([]byte)(s) (spill) at ./main.go:9:20
./main.go:9:20:     from asBytes := ([]byte)(s) (assign) at ./main.go:9:10
./main.go:9:20:   flow: {heap} = asBytes:
./main.go:9:20:     from w.Write(asBytes) (call parameter) at ./main.go:10:9
./main.go:8:12: parameter w leaks to {heap} with derefs=0:
./main.go:8:12:   flow: {heap} = w:
./main.go:8:12:     from w.Write(asBytes) (call parameter) at ./main.go:10:9
./main.go:8:12: leaking param: w
./main.go:8:25: s does not escape
./main.go:9:20: ([]byte)(s) escapes to heap
```
</div>

The compiler claims that `[]byte(s)` escapes because it's being passed to `w.Write`, which is a method on an interface. On the contrary, if we change `w` to `*bytes.Buffer`, the compiler no longer claims an escape:

```go linenums="1"
package main

import (
	"bytes"
)

func print(w *bytes.Buffer, s string) {
	asBytes := []byte(s)
	w.Write(asBytes)
}

func main() {
	buf := bytes.Buffer{}
	print(&buf, "foobar")
}
```
<div class="result">
```title=""
./main.go:7:12: w does not escape
./main.go:7:29: s does not escape
./main.go:8:20: ([]byte)(s) does not escape
```
</div>

### Criteria for Escape

[This repo](https://github.com/akutz/go-interface-values/blob/main/docs/03-escape-analysis/03-escape.md#criteria) provides a wonderful explanation of how escapes actually happen. 

!!! quote "Criteria for Escapes"

    There is one requirement to be eligible for escaping to the heap:

    1. The variable must be a reference type, ex. channels, interfaces, maps, pointers, slices
    2. A value type stored in an interface value can also escape to the heap
    
    If the above criteria is met, then a parameter will escape if it outlives its current stack frame. That usually happens when either:

    1. The variable is sent to a function that assigns the variable to a sink outside the stack frame
    2. Or the function where the variable is declared assigns it to a sink outside the stack frame

Interfaces are a special case of the reference type, because as stated before, the compiler at compile-time has no idea what the implementation of the interface looks like so it has to shortcut its analysis and assume that an escape will happen. The methods defined on other reference types, like `*bytes.Buffer`, can be statically inspected by the analyzer to determine escapes.

### Criteria for Leaks

The linked repo above also explains to us what a leak is.

!!! quote "Criteria for Leaks"

    There are two requirements to be eligible for leaking:

    1. The variable must be a function parameter
    2. The variable must be a reference type, ex. channels, interfaces, maps, pointers, slices
    
    Value types such as built-in numeric types, structs, and arrays are not elgible to be leaked. That does not mean they are never placed on the heap, it just means a parameter of int32 is not going to send you running for a mop anytime soon.

    If the above criteria is met, then a parameter will leak if:

    1. The variable is returned from the same function and/or
    2. is assigned to a sink outside of the stack frame to which the variable belongs.

A leak can also happen without escaping. Consider the case where a value is allocated in frame 0, passed into frame 1 as a pointer, and frame 1 returns that pointer back to frame 0:

```go
package main

import (
	"io"
	"os"
)

func print(w io.Writer, s string) {
	asBytes := []byte(s)
	w.Write(asBytes)
}

func foo(fooString *string) *string {
	return fooString
}

func main() {
	hello := "hello"
	f := foo(&hello)
	print(os.Stdout, *f)
}
```
<div class="result">
```title=""
./main.go:14:2:[1] foo stmt: return fooString
./main.go:13:10: parameter fooString leaks to ~r0 with derefs=0:
./main.go:13:10:   flow: ~r0 = fooString:
./main.go:13:10:     from return fooString (return) at ./main.go:14:2
./main.go:13:10: leaking param: fooString to result ~r0 level=0
```
</div>

We can see it mentions `fooString` leaking, but nowhere does it say it escaped. This is because it knows that the string never escapes from `main` even though the pointer in `foo()` leaks its argument to the return value.

