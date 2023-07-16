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

In this blog post, we discover how you can analyze what variables the Go compiler decides should escape to the heap, a [common source of performance problems](https://tip.golang.org/doc/gc-guide#Eliminating_heap_allocations) in Golang. The gopls language server can be configured to provide insight into which variables are escaping to the heap, and what you can do to prevent that.

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
      "message": "escflow:    flow: ~r0 = &foobar:"
    },
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

What's going on here? `fmt.Print`'s argument is a [variadic string](https://pkg.go.dev/fmt#Print) which basically means we're passing `#!go []string{"foobar"}`, where the string `"foobar"` is the object that we've already shown was allocated to the heap (but this fact is irrelevant to the question of why the slice itself escaped to the heap). We're not returning the address of that object like in our first example, so why is it getting allocated? Do all slices escape to the heap? Let's try with our own simple `print` implementation:

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

The escape analysis says no!

```bash
$ go build -gcflags='-m=3 -json 0,file://out' . |& grep escape
# go-heap-escapes
./main.go:8:25: s does not escape
./main.go:19:7: ... argument does not escape
```

How interesting, so it must be something specific about Go's implementation of `Fprintf`. The source code for this function [is here](https://cs.opensource.google/go/go/+/refs/tags/go1.20.6:src/fmt/print.go;l=260), so let's copy-paste this into our program and see if the compiler can tell what specifically about this implementation causes the escape. The underlying magic happens in the [`doPrint` function](https://cs.opensource.google/go/go/+/refs/tags/go1.20.6:src/fmt/print.go;l=1203) so I ported over an implementation that is largely the same (minus the string format specifier logic, we make an assumption towards the end that the arguments are all strings):

```go linenums="1"
package main

import "reflect"

func doPrint(buf *[]byte, a ...any) {
	prevString := false
	for argNum, arg := range a {
		isString := arg != nil && reflect.TypeOf(arg).Kind() == reflect.String // (1)!
		// Add a space between two non-string arguments.
		if argNum > 0 && !isString && !prevString {
			*buf = append(*buf, byte(' '))
		}
		argAsString := arg.(string) // (2)!
		*buf = append(*buf, []byte(argAsString)...) 
		prevString = isString
	}
}

func main() {
	foo := "foo"
	buf := new([]byte)
	doPrint(buf, foo)
}
```

1. This use of reflection is what the `fmt` package itself uses, so this remains in our port.
2. The remaining code here is changed from the official implementation. The `fmt` implementation has a lot of complicated logic that parses the format specifiers (like `%d` or `%v`) and introduces special handling for each of these. Instead, I just make the assumption that the argument is always a string, so we can assert/cast its type into a string.

What does the GC analysis say?

```bash
go build -gcflags='-m=3 -json 0,file://out' . |& grep escape
# go-heap-escapes
./main.go:14:30: ([]byte)(argAsString) does not escape
./main.go:22:15: foo escapes to heap:
./main.go:21:12: new([]byte) does not escape
./main.go:22:9: ... argument does not escape
./main.go:22:15: foo escapes to heap
```

It escapes, just like `fmt.FPrintf`. The explanation for the escape begins and ends at line 22, but it doesn't provide any useful information as to _why_ it decided that. However, there is an interesting comment it makes about line 5:

```json
{
  "range": {
    "start": {
      "line": 5,
      "character": 27
    },
    "end": {
      "line": 5,
      "character": 27
    }
  },
  "severity": 3,
  "code": "leak",
  "source": "go compiler",
  "message": "parameter a leaks to {heap} with derefs=1",
```

Okay, so something weird is happening inside of `doPrint` with our variadic argument `a`. Let's follow along the `relatedInformation` list to see where it leads us.

```json
      "message": "escflow:    flow: {temp} = a:" // (1)!
      "message": "escflow:    flow: arg = *{temp}:" // (2)!
      "message": "escflow:      from for loop (range-deref)" // (3)!
      "message": "escflow:    flow: reflect.i = arg:" // (4)!
      "message": "escflow:      from reflect.i := arg (assign-pair)" // (5)!
      "message": "escflow:    flow: reflect.eface = reflect.i:" // (6)!
```

1. Line 5
2. Line 7
3. Line 7
4. Line 8
5. Line 8
6. Line 8

The `escflow` then dives into the `reflect` source code itself and meanders through a complicated web of logic, then comes back out into `main.go`, dives into `reflect` again, and back again into `main.go`. The very last comment it makes is:

```json
{
      "location": {
        "uri": "file:///Users/landonclipp/git/newLandonTClipp/LandonTClipp.github.io/code/go-heap-escapes-in-vscode/main.go",
        "range": {
          "start": {
            "line": 8,
            "character": 53
          },
          "end": {
            "line": 8,
            "character": 53
          }
        }
      },
      "message": "escflow:      from .autotmp_6.Kind() (call parameter)"
    }
```

It appears that this long and complicated reason for having escaped ends with calling `.Kind()`. If I remove the use of `.Kind()` and just create some nonsensical code:

```go
		isString := arg != nil && reflect.TypeOf(arg) != nil
```

The GC analyzer then claims that nothing escapes! Fascinating! If we look at the signature of [`reflect.TypeOf`](https://pkg.go.dev/reflect#TypeOf), we see that it returns an interface [`reflect.Type`](https://pkg.go.dev/reflect#Type), where we then call `.Kind()`. This brings us to the next point:

### Interfaces cause heap escapes

This is not a new discovery. It has been known about [for a long time](http://npat-efault.github.io/programming/2016/10/10/escape-analysis-and-interfaces.html) by [multiple different bloggers](https://www.ardanlabs.com/blog/2023/02/interfaces-101-heap-escape.html). It turns out that the Go compiler is incapable of knowing at compile-time whether the method called from an interface somehow retains a copy of its receiver. This is a grammatical definition of the language itself: you have no way to know at compile-time what underlying type is implementing the interface. You can only obtain this information through runtime reflection. The underlying type's receiver may be a concrete value, which means the value is passed-by-value, or it could be a pointer type, which means the receiver can dereference, and potentially save that pointer, somewhere else in memory. It cannot know if it's dealing with a pointer receiver, so it therefore has to assume it's a possibility.

Take for example a simple case:

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

func main() {
	print(os.Stdout, "foobar")
}
```

As you know, `io.Writer` is an interface with a single method `.Write([]byte)`. The escape analyzer again tells us that `[]byte(s)` escapes _because_ it's passed as an argument to `io.Writer`.

![](/images/analyzing-go-heap-escapes/Screenshot 2023-07-15 at 10.10.10 PM.png)

Because we don't know if the thing implementing `io.Writer` is saving a reference of `[]byte(s)` somewhere, it again assumes that it's a possibility.