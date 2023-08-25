---
date: 2023-08-25
categories:
  - programming
  - golang
title: Profile Guided Optimizations in Go
description: How to use PGO to improve the performance of your production applications
draft: true
---

Profile Guided Optimizations in Go
==================================

In this post, we'll explore Profile Guided Optimizations (PGO) introduced in Go 1.20 and how they can be effectively used to improve the performance of your production applications. PGO is a compiler optimization technique that allows you to tune the performance of your Go applications in an environment-specific way. The profiles themselves are simple metadata files that contain information on what functions are called, how often they're called, what system calls are used, and generally what the computational _profile_ is of your application. This information can be used by the compiler to better inform what sorts of optimizations are useful in your specific environment and workfload. 

<!-- more -->

## Why another blog post?

The Go authors [have an excellent blog post](https://go.dev/doc/pgo) on how to use Go's PGO, so what's the point of another blog post? Well, partly because writing blogs is simply a good excuse for me to learn things, and second, to provide a user-facing perspective on PGO and to answer questions on its general usefulness. A lot of optimization questions can be papered over by throwing more horizontal scale at the problem, which is often the approach that people in the high performance computing community take. However there are some scenarios, specifically ones bound by CPU or where the local runtime environment is a scarce resource, where compute optimizations are desirable.

## Optimization overview

The steps you take to profile generally follow this pattern:

1. Build a vanilla go application without any profiling
2. Run the app in production and collect CPU pprof data. This can be done either through an external tool such as perf, through specific call points to [runtime/pprof](https://pkg.go.dev/runtime/pprof), or by hitting an HTTP endpoint in your application by using [net/http/pprof](https://pkg.go.dev/net/http/pprof).
3. Feed the profile into the compiler for a new build
4. Repeat from step 2.

We will explore all of these options and discuss the merits and downfalls of each.

## The Program

The type of workload we're aiming to optimize is a compute-heavy one. Therefore we want to create a program that performs some computationally heavy algorithm. One of the hardest computational problems is the factorization of the product of two large prime numbers, a problem that RSA cryptography relies on being exceedingly expensive for its security guarantees. There are [a large number of algorithms](https://en.wikipedia.org/wiki/Integer_factorization#Factoring_algorithms) we could choose from, but I'll choose [Fermat's Factorization](https://en.wikipedia.org/wiki/Fermat%27s_factorization_method) due to its simplicity.

```go title="main.go" linenums="1"
--8<-- "code/profile-guided-optimizations/fermats-factorization/main.go"
```

<div class="result">
```title=""
$ go build .
$ ./fermants-factorization -n 10976191241513578168
starting CPU profile
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
```
</div>



## Gathering Profiles

### Using test-based pprof

We can generate a CPU profile by simply enabling it during tests. This will give us a rather artificial example of the CPU profile as compared to its real-world behavior, but it might be interesting nonetheless. Let's see what happens.


We run the test benchmarking and tell `go test` to output the cpuprofile to a file named `default.pgo`, which is the default name that `go build` looks for when reading profiles.

```bash
$ go test -cpuprofile default.pgo -memprofile mem.prof -bench . -benchtime 20s
numIterations=1377437 fact1=3314411264 fact2=3311656390
goos: linux
goarch: amd64
pkg: fermats-factorization
cpu: Intel(R) Xeon(R) CPU E5-2699A v4 @ 2.40GHz
BenchmarkFindFactors    numIterations=1377437 fact1=3314411264 fact2=3311656390
       7        3404269889 ns/op
PASS
ok      fermats-factorization  27.268s
```

We can then build our application again and run our command-line profiling to see how the runtime has changed:

```bash
$ go build -pgo=auto .
$ time for i in {1..100} ; do ./fermats-factorization -n 10976191241513578168 ; done
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
[...]

real    5m32.447s
user    5m18.394s
sys     0m0.415s
```

This is a total of 3.32447 seconds on average, which translates to a 2.39% speedup.

### Using in-lined pprof

The method used in `main.go` is to use `runtime/pprof`. The code for that is relatively simple:

```go title="profile.go" linenums="1"
--8<-- "code/profile-guided-optimizations/fermats-factorization/profile.go"
```

It starts the CPU profile and returns a `close` function that the caller must `defer close()` in order to stop the profile and close the file.

### Using HTTP pprof

foobar

## What gets optimized and why?

The Go compiler has this concept called an "inlining budget". The budget controls the maximum number of nodes that a function can have before its considered not inlinable. A node is roughly analagous to the number of nodes in the program's Abstract Syntax Tree (AST), or rather each individual element of a piece of code, like a name declaration, an assignment, a constant declaration, function calls etc. By default, the [`inlineMaxBudget`](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L46) has a value of 80, which means that any functions with more than 80 nodes are not inlinable. If you have profiled your program and the profiler has determined your particular function is "hot", then the [budget increases to 2000](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L76).

### `pgoinlinebudget`

You can modify the inline budget set for hot functions using the `pgoinlinebudget` flag, for example:

```
$ go build -gcflags="-d=pgoinlinebudget=2000" .
```

This is set to 2000 by default but you can specify any value you want. This name is a bit confusing because this does not control the "non-hot" budget, which appears it can't be changed. It _only_ controls the budget for hot functions.

### `pgoinlinecdfthreshold`

The way to read this variable is "Profile Guided Optimization Inline Cumulative Distribution Function Threshold". Wow, what a mouthful! Simply put, this threshold sets the lower bound that the _cumulative weights_ of a function node and all its sub-nodes must have in order to be considered hot. Let's take a look at what this means in practice. We'll set a `pgoinlinecdfthreshold=90` and run the PGO build and graph the DOT notation conveniently provided to us. Note that we've already generated _one_ `default.pgo` profile by simply running the program without any optimizations applied.

```
$ go build .
$ ./fermants-factorization -n 10976191241513578168
starting CPU profile
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
$ go build -pgo=auto -gcflags="-m=2 -pgoprofile=default.pgo -d=pgoinlinecdfthreshold=90,pgodebug=3" .
```

The output of the go build provides a DOT notation graph, seen here:

??? note "DOT graph"
    ```
    digraph G {
        forcelabels=true;
        "os.Create" [color=black, style=solid, label="os.Create,inl_cost=72"];
        "main.main" [color=black, style=solid, label="main.main"];
        "flag.String" [color=black, style=solid, label="flag.String,inl_cost=63"];
        "main.findFactors" [color=black, style=solid, label="main.findFactors"];
        "os.Setenv" [color=black, style=solid, label="os.Setenv,inl_cost=90"];
        "fmt.Println" [color=black, style=solid, label="fmt.Println,inl_cost=72"];
        "flag.Parse" [color=black, style=solid, label="flag.Parse,inl_cost=62"];
        "flag.Uint64" [color=black, style=solid, label="flag.Uint64,inl_cost=63"];
        "main.isSquare" [color=black, style=solid, label="main.isSquare"];
        "fmt.Printf" [color=black, style=solid, label="fmt.Printf,inl_cost=73"];
        "math.Sqrt" [color=black, style=solid, label="math.Sqrt,inl_cost=4"];
        "math.Pow" [color=black, style=solid, label="math.Pow,inl_cost=62"];
        "log.Fatal" [color=black, style=solid, label="log.Fatal"];
        "runtime/pprof.StartCPUProfile" [color=black, style=solid, label="runtime/pprof.StartCPUProfile"];
        "main.profile.func1" [color=black, style=solid, label="main.profile.func1"];
        "os.(*File).Close" [color=black, style=solid, label="os.(*File).Close,inl_cost=67"];
        "main.profile" [color=black, style=solid, label="main.profile"];
        "runtime/pprof.StopCPUProfile" [color=black, style=solid, label="runtime/pprof.StopCPUProfile"];
        "main.NewExpensive" [color=black, style=solid, label="main.NewExpensive"];
        "strconv.Itoa" [color=black, style=solid, label="strconv.Itoa,inl_cost=117"];
        edge [color=red, style=solid];
        "main.isSquare" -> "math.Sqrt" [label="1.86"];
        edge [color=black, style=solid];
        "main.isSquare" -> "main.NewExpensive" [label="0.41"];
        edge [color=red, style=solid];
        "main.isSquare" -> "os.Setenv" [label="1.03"];
        edge [color=black, style=solid];
        "main.isSquare" -> "strconv.Itoa" [label="0.41"];
        edge [color=black, style=solid];
        "main.findFactors" -> "math.Pow" [label="0.21"];
        edge [color=red, style=solid];
        "main.findFactors" -> "main.isSquare" [label="3.92"];
        edge [color=black, style=solid];
        "main.profile" -> "fmt.Println" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile" -> "os.Create" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile" -> "log.Fatal" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile" -> "runtime/pprof.StartCPUProfile" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile" -> "log.Fatal" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile.func1" -> "os.(*File).Close" [label="0.00"];
        edge [color=black, style=solid];
        "main.profile.func1" -> "runtime/pprof.StopCPUProfile" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.String" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Parse" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "main.profile" [label="0.00"];
        edge [color=red, style=solid];
        "main.main" -> "main.findFactors" [label="0.62"];
        edge [color=black, style=solid];
        "main.main" -> "fmt.Printf" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Uint64" [label="0.00"];
    }
    ```

You can copy-paste this into https://dreampuf.github.io/GraphvizOnline/ to visualize it:

![](docs/images/golang-profile-guided-optimizations/graphviz-threshold-90.svg)

You can see here that the PGO determined all of the paths in red are considered hot because their weights exceed the calculated hot callsite threshold:

```
$ go build -pgo=auto -gcflags="-m=3 -pgoprofile=default.pgo -d=pgoinlinecdfthreshold=90,pgodebug=3" . |& grep hot-callsite-thres-from-CDF
hot-callsite-thres-from-CDF=0.4123711340206186
```

If we decrease the pgoinlinecdfthreshold value to something like 80, we see a dramatically different result:

```
$ go build -pgo=auto -gcflags="-m=3 -pgoprofile=default.pgo -d=pgoinlinecdfthreshold=80,pgodebug=3" . |& grep hot-callsite-thres-from-CDF
hot-callsite-thres-from-CDF=1.443298969072165
```

And the visualization shows us that now only two of the call paths are considered hot because only two of the weights are above 1.443298969072165:

![](/images/golang-profile-guided-optimizations/graphviz-threshold-80.png)

Remember that the PGO will select the edge weights whose cumulative distribution in "weight", or total program runtime, adds up to the percentage set by `pgoinlinecdfthreshold`. This is why setting a higher percentage includes more weights because their total runtime distribution will be at or above this value.


## Viewing the assembly

Let's have some fun an convince ourselves on what's really going on here. Sure these nice pretty graphs tell us that the PGO has inlined certain function calls, but why don't we take a look at the raw assembly code? First, let's look at the unoptimzed executable by building it with PGO turned off:

```
$ go build -pgo=off 
$ go tool objdump ./fermants-factorization |& less
```

By grepping for `main.go:33` we indeed find the location where `main.isSquare` is called on the function stack:

```
  main.go:33            0x4ae2f6                e825feffff              CALL main.isSquare(SB)
```

Let's build this again with PGO turned on, and for fun let's just rely on the default PGO values:

```
$ go build -pgo=auto -gcflags="-m=3 -pgoprofile=default.pgo -d=pgodebug=1" .  |& grep isSquare
hot-node enabled increased budget=2000 for func=main.isSquare
./main.go:11:6: can inline isSquare with cost 370 as: func(uint64) (bool, uint64) { sqrt := math.Sqrt(float64(i)); expensive := NewExpensive(); os.Setenv("EXPENSIVE_VALUE", strconv.Itoa(int(expensive))); return sqrt == float64(uint64(sqrt)), uint64(sqrt) }
hot-budget check allows inlining for call main.NewExpensive (cost 130) at ./main.go:17:27 in function main.isSquare
hot-budget check allows inlining for call strconv.Itoa (cost 117) at ./main.go:18:43 in function main.isSquare
hot-budget check allows inlining for call os.Setenv (cost 90) at ./main.go:18:11 in function main.isSquare
hot-budget check allows inlining for call main.isSquare (cost 370) at ./main.go:33:29 in function main.findFactors
./main.go:33:29: inlining call to isSquare
```

Great! Even with the default parameters it still shows `main.isSquare` is allowed to be inlined. The graph visualization agrees:

![](/images/golang-profile-guided-optimizations/graphviz-threshold-defaults.svg)

What does the assembly say?

```
  main.go:33            0x4af96e                e82df9ffff              CALL main.isSquare(SB)     
```

Oh no! :scream: What happened? Let's take a closer look at what the inliner actually did:

```
./main.go:11:6: can inline isSquare with cost 370 as: func(uint64) (bool, uint64) { sqrt := math.Sqrt(float64(i)); expensive := NewExpensive(); os.Setenv("EXPENSIVE_VALUE", strconv.Itoa(int(expensive))); return sqrt == float64(uint64(sqrt)), uint64(sqrt) }
```

Okay so the `isSquare` call itself doesn't actually get inlined, despite what the inline optimization tells us. However, we can compare all of the CALL instructions to see what got axed:

```
$ go build -pgo=off .
$ go tool objdump ./fermats-factorization |& grep CALL | sort -u |& grep main.go
  main.go:11            0x4ae220                e83b01fbff              CALL runtime.morestack_noctxt.abi0(SB)
  main.go:17            0x4ae164                e897ffffff              CALL main.NewExpensive(SB)
  main.go:18            0x4ae185                e896a1fdff              CALL os.Setenv(SB)
  main.go:23            0x4ae34b                e81000fbff              CALL runtime.morestack_noctxt.abi0(SB)
  main.go:33            0x4ae2f6                e825feffff              CALL main.isSquare(SB)
  main.go:40            0x4ae340                e89b37f8ff              CALL runtime.gopanic(SB)
  main.go:43            0x4ae581                e8dafdfaff              CALL runtime.morestack_noctxt.abi0(SB)
  main.go:49            0x4ae451                e84a010000              CALL main.profile(SB)
  main.go:52            0x4ae46b                e8d0fdffff              CALL main.findFactors(SB)
  main.go:53            0x4ae4a0                e89bc2f5ff              CALL runtime.convT64(SB)
  main.go:53            0x4ae4c0                e87bc2f5ff              CALL runtime.convT64(SB)
  main.go:53            0x4ae4e0                e85bc2f5ff              CALL runtime.convT64(SB)
  main.go:53            0x4ae501                e83ac2f5ff              CALL runtime.convT64(SB)
  main.go:55            0x4ae55a                ffd6                    CALL SI
$ go build -pgo=auto -gcflags="-m=3 -pgoprofile=default.pgo -d=pgodebug=1" . &>/dev/null
$ go tool objdump ./fermats-factorization |& grep CALL | sort -u |& grep main.go
  main.go:11            0x4af3a7                e87403fbff              CALL runtime.morestack_noctxt.abi0(SB)
  main.go:33            0x4af96e                e82df9ffff              CALL main.isSquare(SB)
  main.go:40            0x4afae8                e8332ef8ff              CALL runtime.gopanic(SB)
  main.go:43            0x4afb08                e813fcfaff              CALL runtime.morestack_noctxt.abi0(SB)
  main.go:49            0x4af4b4                e867060000              CALL main.profile(SB)
  main.go:53            0x4af9f4                e887adf5ff              CALL runtime.convT64(SB)
  main.go:53            0x4afa20                e85badf5ff              CALL runtime.convT64(SB)
  main.go:53            0x4afa4a                e831adf5ff              CALL runtime.convT64(SB)
  main.go:53            0x4afa73                e808adf5ff              CALL runtime.convT64(SB)
  main.go:55            0x4afacf                ffd1                    CALL CX

```

As we can see, the things that did get inlined were:

1. `main.go:17`
2. `main.go:18`
3. `main.go:23`
4. `main.go:52`

The astute observer may notice that both the visualization graph and the PGO itself claimed that `main.isSquare` got inlined, but the message tells us it just got inlined as another function. I won't go as far as to claim that it's a bug in the compiler but it's certainly not expected.


The 

The default threshold for hot inlining is 99%: https://github.com/golang/go/blob/e92c0f846c54d88f479b1c48f0dbc001d2ff53e9/src/cmd/compile/internal/inline/inl.go#L74

This can be modified using something like:

```
go build -pgo=auto -gcflags="-m -m -m -pgoprofile=default.pgo -d=pgoinlinebudget=2000,pgoinlinecdfthreshold=0"
```

You can print PGO debug statements like:

```
(ve) [lclipp@fpif-vcsl1 fermats-factorization][0] $ go build -pgo=auto -gcflags="-m -pgoprofile=default.pgo -d=pgoinlinebudget=2000,pgoinlinecdfthreshold=1,pgodebug=3" . 
```

You can visualize the dot graph by copying the DOT format here: https://dreampuf.github.io/GraphvizOnline/

!!! tip
    This allowed me to make isSquare inlined:

    ```
    $ ./fermats-factorization -n 10976191241513578168
    $ go build -pgo=auto -gcflags="-m=2 -pgoprofile=default.pgo -d=pgoinlinebudget=2000,pgoinlinecdfthreshold=90,pgodebug=3" .  |& grep isSquare
    ```

    The trick was setting pgoinlinecdfthreshold to a high enough percentage value so that the function is considered "hot", and setting pgoinlinebudget to a high enough value so that our budget for hot functions is high enough. 

    The value from the debug output here:

    ```
    hot-callsite-thres-from-CDF=0.36993769470404986
    ```

    is inversely correllated to the pgoinlinecdfthreshold value. 

### Dissecting the original binary

By disassembling the original binary, we can see that the call to `isSquare` is not inlined. This is by design actually, as we added an "expensive" call to `NewExpensive()` that caused `isSquare`'s total node count to exceed Go's budget of 80. You can see the compiler tell us this by using verbose GC flags:

```bash
$ go build -pgo=off -gcflags="-m -m -m" . |& grep isSquare
./main.go:11:6: cannot inline isSquare: function too complex: cost 84 exceeds budget 80
```

By doing an objdump, we can also see this fact in the assembly code:

```bash
$ go tool objdump ./fermats-factorization |& grep 'main.go:30'
  main.go:30            0x4887f1                e82afeffff              CALL main.isSquare(SB)       
```

Keep this fact in mind for the following section.

### Dissecting the profile-guided binary

Using the results from the test-driven profile, we can disassemble the executable and look at what optimization decisions the go compiler made. Let's take a look at the line where we call out to `isSquare`:

```go
issquare, sqrt := isSquare(uint64(potentialSquare))
```

```bash
$ go tool objdump ./fermats-factorization
```
<div class="result">
```title=""
  main.go:22            0x4878d9                90                      NOPL                                    
  main.go:10            0x4878da                4885d2                  TESTQ DX, DX                            
  main.go:10            0x4878dd                7c0a                    JL 0x4878e9                             
  main.go:10            0x4878df                0f57db                  XORPS X3, X3                            
  main.go:10            0x4878e2                f2480f2ada              CVTSI2SDQ DX, X3                        
  main.go:10            0x4878e7                eb18                    JMP 0x487901                            
  main.go:10            0x4878e9                4889d7                  MOVQ DX, DI                             
  main.go:10            0x4878ec                83e201                  ANDL $0x1, DX                           
  main.go:10            0x4878ef                48d1ef                  SHRQ $0x1, DI                           
  main.go:10            0x4878f2                4809d7                  ORQ DX, DI                              
  main.go:10            0x4878f5                0f57db                  XORPS X3, X3                            
  main.go:10            0x4878f8                f2480f2adf              CVTSI2SDQ DI, X3                        
  main.go:10            0x4878fd                f20f58db                ADDSD X3, X3                            
  sqrt.go:94            0x487901                f20f51db                SQRTSD X3, X3                           
  main.go:11            0x487905                660f2ed3                UCOMISD X3, X2            
```
</div>

We can see that the entire flow of function calls have been completely inlined. There are no `CALL` instructions to `isSquare`, and even the call to `math.Sqrt` has been inlined.



## Junkyard

### test-based pprof


We can write a simple benchmarking test for `findFactors`

```go title="main_test.go"
--8<-- "code/profile-guided-optimizations/fermats-factorization/main_test.go"
```

And then run the benchmark for 20 seconds and get an average result

```bash
$ go test -bench=. -benchtime 20s

numIterations=1377437 fact1=3314411264 fact2=3311656390

goos: linux
goarch: amd64
pkg: fermats-factorization
cpu: Intel(R) Xeon(R) CPU E5-2699A v4 @ 2.40GHz
BenchmarkFindFactors 

numIterations=1377437 fact1=3314411264 fact2=3311656390
numIterations=1377437 fact1=3314411264 fact2=3311656390

     681          32507910 ns/op
PASS
ok      fermats-factorization  25.699s
```

This is useful, but we want to find the average execution time of the executable, not the test. So instead we can run it 100 times and average the result:

```bash
$ time for i in {1..100} ; do ./fermats-factorization -n 10976191241513578168 ; done
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
[...]

real    5m40.590s
user    5m17.049s
sys     0m0.564s
```

This gives us an average runtime of 3.4059 seconds.
