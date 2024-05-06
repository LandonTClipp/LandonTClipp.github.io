---
title: Golang
---

Golang
=======

Garbage Collector
------------------

https://tip.golang.org/doc/gc-guide

Go is a garbage collected language. When a new variable is allocated (either with `make` or through literal declarations), the Go escape analyzer (which is run at compile time) determines if a particular variable should escape to the heap, or if it can remain on the stack. The garbage collector cleans heap-allocated data.

OOP (in Go)
-----------

The four pillars of OOP:

1. Polymorphism
2. Abstraction
3. Encapsulation
4. Inheritance

Go has all of the pillars except for 4. A common misconception is that Go's [type embedding](https://go.dev/doc/effective_go#embedding) constitutes inheritance, but embedding is only syntatic sugar around composition. For example, an embedded type does not have access to the "children" structs. Thus, it doesn't have true inheritance.

Goroutines
-----------

A goroutine is essentially a "thread" of execution (not to be confused with an OS thread) that is managed by the Go runtime. goroutines are multiplexed and scheduled onto OS threads according to the needs of the goroutine. goroutines that are waiting on IO are put into a sleep state until they are ready to handle the IO.

Functional Options
-------------------

Functional Options are a style of specifying optional arguments. This is needed to get around the fact that Go does not have default parameter values. It works by creating a type that is defined as a function that takes, as its input a config struct, and returns nothing. The function will set a specific attribute in the config to the value passed in to the constructor. For example, `#!go type FuncOpt func(c *Config)`. Here is an example:

```go
package main

import "fmt"

type Person struct {
	name   string
	age    int
	height int
}

type PersonOpt func(p *Person)

func PersonName(name string) PersonOpt {
	return func(p *Person) {
		p.name = name
	}
}

func PersonAge(age int) PersonOpt {
	return func(p *Person) {
		p.age = age
	}
}

func PersonHeight(height int) PersonOpt {
	return func(p *Person) {
		p.height = height
	}
}

func NewPerson(opts ...PersonOpt) Person {
	person := Person{
		name:   "Landon",
		age:    2,
		height: 500,
	}

	for _, opt := range opts {
		opt(&person)
	}
	return person
}
func main() {
	person := NewPerson(PersonHeight(175))
	fmt.Printf("%+v\n", person)
}
```
<div class="result">
```title=""
{name:Landon age:2 height:175}
```
</div>

The meat of this methodology lies within the `PersonOpt` constructor. Let's zoom in:

```go
type Person struct {
	name   string
	age    int
	height int
}

type PersonOpt func(p *Person)

func PersonName(name string) PersonOpt {
	return func(p *Person) {
		p.name = name
	}
}
```

You can see that `PersonName` is one such constructor that takes in a string, and it returns a function that modifies the value of the `Person` struct with the value "`name`" passed into the `PersonOpt` constructor. It relies on the fact that [Go functions can be closures](https://go.dev/tour/moretypes/25)

Tools as Dependencies
----------------------

stub

TODO: Show the `tools/` sub-module approach to setting tool dependencies.

Types
-----

### rune

A rune is an alias for `int32`. It represents a code point in the Unicode standard. The elements of a string (which are just singular bytes) can be converted to a rune through a simple `#!go rune(elem)` type conversion. It's important to know that some characters in the Unicode specification can be represented by different bytes. For example, à can be represented in two ways: either it's explicit code point U+00E0, or through a sequence of two separate code points of U+0061 (which is the lowercase `a`) and U+0300 (which is the "combining" grave access point, which can be applied to any character). This form of character representation would require two elements in a string type in Go because it requires two bytes, but can be easily represented by a single rune.

### Channels

Receive

```go
foo := <-chanVar
```

Send

```go
chanVar <- 5
```

Channel directions

```go
func receiveOnly(chanVar <-chan int){}
func sendOnly(chanVar chan<- int){}
func sendOrReceive(chanVar chan int){}
```

### Maps

```go
mapVar := map[string]int{}
mapVar = make(map[string]int)
```

### Slices

https://go.dev/blog/slices-intro

Slices are a metadata view over a dynamically-sized array. A slice contains:

1. A reference to the underlying array
2. The length of our view in the underlying array (which serves as a boundary)
3. A capacity, which is the the number of elements between our pointer to the underlying array, and its end.

Slices are automatically expanded by the runtime when we run out of capacity. It does this by first allocating a new array that is twice the size of the original, then copies the contents of the new array. You can see this expansion behavior as such:

```go
package main

import "fmt"

func main() {
	s := []int{}
	for i := 0; i < 100; i++ {
		s = append(s, i)
		fmt.Printf("length: %v capacity: %v\n", len(s), cap(s))
	}
}
```
<div class="result">
```title=""
length: 1 capacity: 1
length: 2 capacity: 2
length: 3 capacity: 4
length: 4 capacity: 4
length: 5 capacity: 8
length: 6 capacity: 8
length: 7 capacity: 8
length: 8 capacity: 8
length: 9 capacity: 16
length: 10 capacity: 16
length: 11 capacity: 16
length: 12 capacity: 16
...
```
</div>

When you allocate slices, you can specify the capacity of the underlying array:

```go
package main

import "fmt"

func main() {
	s := make([]int, 0, 100)
	for i := 0; i < 100; i++ {
		s = append(s, i)
		fmt.Printf("length: %v capacity: %v\n", len(s), cap(s))
	}
}
```
<div class="result">
```title=""
length: 1 capacity: 100
length: 2 capacity: 100
length: 3 capacity: 100
length: 4 capacity: 100
```
</div>

## Preferred Readings

### Go Interface Values Deep Dive


https://github.com/akutz/go-interface-values/tree/main

### A Guide to the Go Garbage Collector

https://tip.golang.org/doc/gc-guide

### Faster software through register based calling

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



### Generics Degrade Performance

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

