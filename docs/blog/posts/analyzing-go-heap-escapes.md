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

Configuring VSCode for GC Heap Escape Highlighting
------------------------------

You can configure VSCode to highlight cases of heap escapes:

![](/images/analyzing-go-heap-escapes/Screenshot 2023-07-16 at 3.10.58 AM.png)

The [VSCode plugin for Go](https://code.visualstudio.com/docs/languages/go) provides integrations with its `gopls` language server. The language server is simply a subprocess that VSCode calls and creates a UNIX pipe through which queries and responses to the server can be sent (you can also run gopls as a TCP server listening to a local port). `gopls` can be configured in your VSCode workspace settings to highlight instances of heap escape in your code.

If VSCode has not yet created a workspace for your project, open `File`/`Save Workspace As`, and save a workspace file in the root of your project. The configuration I used for this blog is this:

```yaml title="workspace.json"
{
    "folders": [
        {
            "path": "."
        }
    ],
    "settings": {
        "go.enableCodeLens": {
            "runtest": true # (1)!
        },
        "gopls": {
            "ui.codelenses": { # (2)!
                "generate": true,
                "gc_details": true  # (3)!
            },
            "ui.diagnostic.annotations": {
                "escape": true # (4)!
            },
            "ui.semanticTokens": true
        },
    }
}
```

1. This is not relevant to GC highlighting, but is useful for Codelenses for unit tests
2. These are the parameters you need to enable general GC annotations
3. This enables a Codelens option for toggling the GC decisions on/off
4. This actually enables the escape annotations

After doing this, hovering your mouse over the highlights show the results of the escape analysis:

![](/images/analyzing-go-heap-escapes/Screenshot 2023-07-16 at 3.12.03 AM.png)

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
$ go build -gcflags='-m=3' . |& grep escape
# go-heap-escapes
./main.go:6:2: foobar escapes to heap:
./main.go:12:12: *r escapes to heap:
./main.go:12:11: ... argument does not escape
./main.go:12:12: *r escapes to heap
```

You can also specify `go build -gcflags='-m=3 -json=file://out' .` to have it print the results to a number of json files.

Looking closer at the output, we see these messages:

```
./main.go:6:2: foobar escapes to heap:
./main.go:6:2:   flow: ~r0 = &foobar:
./main.go:6:2:     from &foobar (address-of) at ./main.go:7:9
./main.go:6:2:     from return &foobar (return) at ./main.go:7:2
./main.go:6:2: moved to heap: foobar
```

This is telling us very clearly that `foobar escapes to heap`. It's pretty obvious why, let's take a closer look.

```go linenums="1"
func foobar() *string {
	foobar := "foobar"
	return &foobar
}
```

This function instantiates a string named `foobar` that has the value `"foobar"`. We return a pointer to `foobar` which then means that the string initially allocated on the stack of `func foobar() *string` can now be referenced by functions outside of this lexical scope. So the variable can't remain on the stack; it _has_ to escape to the heap.

The message even tells us exactly why it escapes and what sequences of events had to happen for it to escape. By following the escape flow messages, we can see the three necessary events were:

```
./main.go:6:2:     from &foobar (address-of) at ./main.go:7:9
./main.go:6:2:     from return &foobar (return) at ./main.go:7:2
```

It escapes because we:

1. We took the address of `foobar`
2. We returned that address

### Escapes of `fmt.Print`

!!! todo

    TODO: this needs to be fleshed out. The point is not driven home yet. Explain this: https://cs.opensource.google/go/go/+/refs/tags/go1.20.6:src/fmt/print.go;l=1203

We also see that there's another escape on line 12 in the `fmt.Print`. 

```
./main.go:12:12: *r escapes to heap:
./main.go:12:12:   flow: {storage for ... argument} = &{storage for *r}:
./main.go:12:12:     from *r (spill) at ./main.go:12:12
./main.go:12:12:     from ... argument (slice-literal-element) at ./main.go:12:11
./main.go:12:12:   flow: fmt.a = &{storage for ... argument}:
./main.go:12:12:     from ... argument (spill) at ./main.go:12:11
./main.go:12:12:     from fmt.a := ... argument (assign-pair) at ./main.go:12:11
./main.go:12:12:   flow: {heap} = *fmt.a:
./main.go:12:12:     from fmt.Fprint(os.Stdout, fmt.a...) (call parameter) at ./main.go:12:11
./main.go:12:11: ... argument does not escape
./main.go:12:12: *r escapes to heap
```

What's going on here? `fmt.Print`'s argument is a [variadic argument of type `any`](https://pkg.go.dev/fmt#Print) which basically means we're passing `#!go []any{"foobar"}`, where the string `"foobar"` is the object that we've already shown was allocated to the heap (but this fact is irrelevant to the question of why the slice itself escaped to the heap). `any` is just an alias for `interface{}` so we're passing a slice of interfaces. Why is this a problem? Let's try with our own simple `print` implementation that instead uses `...string`:

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
<div class="result">
```bash
$ go build -gcflags='-m=3' .
./main.go:8:12: parameter w leaks to {heap} with derefs=0:
./main.go:8:12:   flow: {heap} = w:
./main.go:8:12:     from w.Write(buf) (call parameter) at ./main.go:15:9
./main.go:8:12: leaking param: w
./main.go:8:25: s does not escape
./main.go:19:6:[1] main stmt: foo := "foo"
./main.go:19:2:[1] main stmt: var foo string
./main.go:20:7:[1] main stmt: print(os.Stdout, foo)
./main.go:20:7: ... argument does not escape
```
</div>

Does this escape? The analysis says no!

What you will see is that it claims `w` "leaks" to the heap, but that does not necessarily mean it's going to be allocated on the heap itself. It simply means that it has the _potential_ to leak onto the heap. Whether or not it does depends on the parent stack frames.

Let's take a closer look at what `fmt.Print`is doing. Under the hood, it calls [this function](https://cs.opensource.google/go/go/+/refs/tags/go1.20.6:src/fmt/print.go;l=1203). We can modify this to see what exactly causes the escape.

```go linenums="1" hl_lines="22"
package main

import (
	"bytes"
	"reflect"
)

func doPrint(b *bytes.Buffer, a []any) {
	prevString := false
	for argNum, arg := range a {
		isString := arg != nil && reflect.TypeOf(arg).Kind() == reflect.String
		// Add a space between two non-string arguments.
		if argNum > 0 && !isString && !prevString {
			b.WriteByte(' ')
		}
		prevString = isString
	}
}

func main() {
	w := bytes.Buffer{}
	doPrint(&w, []any{"foobar"})
}
```
<div class="result">
```title=""
./main.go:8:14: b does not escape
./main.go:22:20: "foobar" escapes to heap:
./main.go:22:19: []any{...} does not escape
./main.go:22:20: "foobar" escapes to heap
```
</div>

After some investigation, I found that simply removing the `.Kind()` call results in no escapes happening:

```go linenums="1"
package main

import (
	"bytes"
	"reflect"
)

func doPrint(b *bytes.Buffer, a []any) {
	prevString := false
	for argNum, arg := range a {
		isString := arg != nil && reflect.TypeOf(arg) == reflect.TypeOf(arg) // (1)!
		// Add a space between two non-string arguments.
		if argNum > 0 && !isString && !prevString {
			b.WriteByte(' ')
		}
		prevString = isString
	}
}

func main() {
	w := bytes.Buffer{}
	doPrint(&w, []any{"foobar"})
}
```

1. We do this `reflect.TypeOf` check to ensure that `reflect.TypeOf` is actually used. Our goal is to keep the `reflect.TypeOf` call but not the `.Kind()`

```title=""
./main.go:8:14: b does not escape
./main.go:8:31: a does not escape
./main.go:22:19: []any{...} does not escape
./main.go:22:20: "foobar" does not escape
```

This brings us to the next point:

### Situations which cause escapes

#### Use of interfaces

This is not a new discovery. It has been known about [for a long time](http://npat-efault.github.io/programming/2016/10/10/escape-analysis-and-interfaces.html) by [multiple different bloggers](https://www.ardanlabs.com/blog/2023/02/interfaces-101-heap-escape.html). It turns out that the Go compiler is incapable of knowing at compile-time whether the underlying type in an interface is a value type or a pointer. As we already know, the compiler has to assume that a pointer's dereferenced value can outlive the life of its lexical scope, so it has to make the conservative assumption that the slice can contain pointers.

We can see this is true even in the simple case where the argument is a bare interface:

```go linenums="1" hl_lines="9"
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

#### Use of reflection

Going back to our [previous example](#escapes-of-fmtprint) where `fmt.Print` caused an escape, we showed that this happens because of the `reflect.TypeOf(arg).Kind()` call:

```go linenums="1" hl_lines="15 14 11 20"
package main

import (
	"bytes"
	"fmt"
	"reflect"
)

func doPrint(b *bytes.Buffer, a ...any) {
	randomBool := reflect.TypeOf(a) == reflect.TypeOf(a)
	kind := reflect.TypeOf(a).Kind()

	b.WriteByte(byte(0))
	fmt.Print(randomBool)
	fmt.Printf("%v", kind)
}

func main() {
	w := bytes.Buffer{}
	doPrint(&w, "foobar")
}
```
<div class="result">
```title=""
./main.go:15:19: kind escapes to heap:
./main.go:14:12: randomBool escapes to heap:
./main.go:11:25: a escapes to heap:
./main.go:9:14: b does not escape
./main.go:10:31: a does not escape
./main.go:10:52: a does not escape
./main.go:11:25: a escapes to heap
./main.go:14:11: ... argument does not escape
./main.go:14:12: randomBool escapes to heap
./main.go:15:12: ... argument does not escape
./main.go:15:19: kind escapes to heap
./main.go:20:9: ... argument escapes to heap:
./main.go:20:14: "foobar" escapes to heap:
./main.go:20:9: ... argument escapes to heap
./main.go:20:14: "foobar" escapes to heap
```
</div>

#### Use of reference types on interface methods

What if we were to modify our example so that we're sending a value type to the method, instead of a reference type?

```go linenums="1"
package main

type Writer interface {
	Write(b string) (int, error)
}

type writer struct{}

func (w writer) Write(b string) (int, error) {
	return 0, nil
}

func print(w Writer) {
	s := "foobar"
	w.Write(s)
}

func main() {
	var w Writer = writer{}
	print(w)
}
```
<div class="result">
```title=""
./main.go:9:23: b does not escape
./main.go:19:23: writer{} does not escape
```
</div>

And going back to a reference type, we can see yet again that simply changing the argument to a reference type causes it to escape.

```go linenums="1" hl_lines="15"
package main

type Writer interface {
	Write(b []byte) (int, error)
}

type writer struct{}

func (w writer) Write(b []byte) (int, error) {
	return 0, nil
}

func print(w Writer) {
	s := "foobar"
	b := []byte(s)
	w.Write(b)
}

func main() {
	var w Writer = writer{}
	print(w)
}
```
<div class="result">
```title=""
./main.go:9:23: b does not escape
./main.go:15:14: ([]byte)(s) escapes to heap:
./main.go:15:14: ([]byte)(s) escapes to heap
./main.go:20:23: writer{} does not escape
./main.go:21:7: ([]byte)(s) does not escape
```
</div>

What if we use a reference type on a non-interface value? Instead of `print` taking a `Writer` interface, we modify it to take a `writer` struct directly:

```go
package main

type writer struct{}

func (w writer) Write(b []byte) (int, error) {
	return 0, nil
}

func print(w writer) {
	s := "foobar"
	b := []byte(s)
	w.Write(b)
}

func main() {
	print(writer{})
}
```
<div class="result">
```title=""
./main.go:5:23: b does not escape
./main.go:11:14: ([]byte)(s) does not escape
./main.go:16:7: ([]byte)(s) does not escape
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

```go linenums="1"
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

Conclusions
-----------

These experiments lead us to conclude a few main points:

1. Sending reference types as an argument to an interface method causes heap escapes
2. Interface types themselves can escape, 