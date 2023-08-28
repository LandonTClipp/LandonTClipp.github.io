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
$ ./fermats-factorization -n 10976191241513578168
starting CPU profile
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
```
</div>

## The Benchmark

The benchmark we'll be using is going to be captured by timing the program's runtime over 100 iterations without any optimizations applied. We'll allow the program to generate a profile during each iteration, but we won't use it when building. 

```
$ go build -pgo=off .
$ time for i in {1..100} ; do ./fermats-factorization -n 10976191241513578168 -cpuprofile="" ; done
real    5m53.244s
user    5m13.847s
sys     0m1.101s
```

Dividing the real time by 100 gives us 3.53244 seconds. This will be the value we use to compare the effectivess of our PGO builds.

## Gathering Profiles

### Using test-based pprof

We can generate a CPU profile by simply enabling it during tests. This will give us a rather artificial example of the CPU profile as compared to its real-world behavior, but it might be interesting nonetheless. Let's see what happens.

We can write a simple benchmarking test for `findFactors`:

```go title="main_test.go"
--8<-- "code/profile-guided-optimizations/fermats-factorization/main_test.go"
```


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

The Go compiler has this concept called an "inlining budget". The budget controls the maximum number of syntatical nodes that a function can have before its considered not inlinable. A node is roughly analagous to the number of nodes in the program's Abstract Syntax Tree (AST), or rather each individual element of a piece of code, like a name declaration, an assignment, a constant declaration, function calls etc. By default, the [`inlineMaxBudget`](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L46) has a value of 80, which means that any functions with more than 80 nodes are not inlinable. If you have profiled your program and the profiler has determined your particular function is "hot", then the [budget increases to 2000](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L76).

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
$ ./fermats-factorization -n 10976191241513578168
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

![](/images/golang-profile-guided-optimizations/graphviz-threshold-90.svg)

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

Mathematically, the cumulative distribution function maps `x`, or the `hot-callsite-thres-from-CDF` value, to the `y` value of `pgoinlinecdfthreshold`. It chooses the weight that corresponds to the runtime percentage you specify such that only the methods which eat `pgoinlinecdfthreshold` percentage of the runtime are considered hot.


## Viewing the assembly

Let's have some fun an convince ourselves on what's really going on here. Sure these nice pretty graphs tell us that the PGO has inlined certain function calls, but why don't we take a look at the raw assembly code? First, let's look at the unoptimzed executable by building it with PGO turned off:

```
$ go build -pgo=off 
$ go tool objdump ./fermats-factorization |& less
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

1. `main.go:17`: `CALL main.NewExpensive(SB)`
2. `main.go:18`: `CALL os.Setenv(SB)`
3. `main.go:23`: `CALL runtime.morestack_noctxt.abi0(SB)`
4. `main.go:52`: `CALL main.findFactors(SB)`

<div class="annotate" markdown>

Both the visualization graph and the PGO itself claimed that `main.isSquare` got inlined, but the message tells us it just got inlined as another function. It's clear that the calls within `isSquare` themselves are getting inlined, but `isSquare` itself when called from `findFactors` does not. To add to the confusion, the inline analysis node chart shows the function call being inlined too:

??? note "inline analysis"

    ```yaml title=""
    hot-budget check allows inlining for call main.isSquare (cost 370) at ./main.go:33:29 in function main.findFactors
    ./main.go:33:29: inlining call to isSquare
    ./main.go:33:29: Before inlining: 
    .   CALLFUNC STRUCT-(bool, uint64) tc(1) # main.go:33:29
    .   .   NAME-main.isSquare Class:PFUNC Offset:0 Used FUNC-func(uint64) (bool, uint64) tc(1) # main.go:11:6
    .   CALLFUNC-Args
    .   .   NAME-main.potentialSquare Class:PAUTO Offset:0 OnStack Used uint64 tc(1) # main.go:26:3
    ./main.go:33:29: After inlining 
    .   INLCALL-init
    .   .   AS2-init
    .   .   .   DCL # main.go:33:29
    .   .   .   .   NAME-main.i Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:15
    .   .   AS2 Def tc(1) # main.go:33:29
    .   .   AS2-Lhs
    .   .   .   NAME-main.i Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:15
    .   .   AS2-Rhs
    .   .   .   NAME-main.potentialSquare Class:PAUTO Offset:0 OnStack Used uint64 tc(1) # main.go:26:3
    .   .   INLMARK Index:6 # +main.go:33:29
    .   INLCALL STRUCT-(bool, uint64) tc(1) # main.go:33:29
    .   INLCALL-Body
    .   .   AS-init
    .   .   .   DCL # main.go:33:29 main.go:12:2
    .   .   .   .   NAME-main.sqrt Class:PAUTO Offset:0 InlLocal OnStack Used float64 tc(1) # main.go:33:29 main.go:12:2
    .   .   AS Def tc(1) # main.go:33:29 main.go:12:7
    .   .   .   NAME-main.sqrt Class:PAUTO Offset:0 InlLocal OnStack Used float64 tc(1) # main.go:33:29 main.go:12:2
    .   .   .   CALLFUNC float64 tc(1) # main.go:33:29 main.go:12:19
    .   .   .   .   NAME-math.Sqrt Class:PFUNC Offset:0 Used FUNC-func(float64) float64 tc(1) # sqrt.go:93:6
    .   .   .   CALLFUNC-Args
    .   .   .   .   CONV float64 tc(1) # main.go:33:29 main.go:12:28
    .   .   .   .   .   NAME-main.i Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:15
    .   .   AS-init
    .   .   .   DCL # main.go:33:29 main.go:17:2
    .   .   .   .   NAME-main.expensive Class:PAUTO Offset:0 InlLocal OnStack Used main.Expensive tc(1) # main.go:33:29 main.go:17:2
    .   .   AS Def tc(1) # main.go:33:29 main.go:17:12
    .   .   .   NAME-main.expensive Class:PAUTO Offset:0 InlLocal OnStack Used main.Expensive tc(1) # main.go:33:29 main.go:17:2
    .   .   .   CALLFUNC main.Expensive tc(1) # main.go:33:29 main.go:17:27
    .   .   .   .   NAME-main.NewExpensive Class:PFUNC Offset:0 Used FUNC-func() Expensive tc(1) # expensive.go:5:6
    .   .   CALLFUNC error tc(1) # main.go:33:29 main.go:18:11
    .   .   .   NAME-os.Setenv Class:PFUNC Offset:0 Used FUNC-func(string, string) error tc(1) # env.go:119:6
    .   .   CALLFUNC-Args
    .   .   .   LITERAL-"EXPENSIVE_VALUE" string tc(1) # main.go:33:29 main.go:18:12
    .   .   .   CALLFUNC string tc(1) # main.go:33:29 main.go:18:43
    .   .   .   .   NAME-strconv.Itoa Class:PFUNC Offset:0 Used FUNC-func(int) string tc(1) # itoa.go:34:6
    .   .   .   CALLFUNC-Args
    .   .   .   .   CONV int tc(1) # main.go:33:29 main.go:18:48
    .   .   .   .   .   NAME-main.expensive Class:PAUTO Offset:0 InlLocal OnStack Used main.Expensive tc(1) # main.go:33:29 main.go:17:2
    .   .   BLOCK tc(1) # main.go:33:29
    .   .   BLOCK-List
    .   .   .   DCL tc(1) # main.go:33:29
    .   .   .   .   NAME-main.~R0 Class:PAUTO Offset:0 InlFormal OnStack Used bool tc(1) # main.go:33:29 main.go:11:26
    .   .   .   DCL tc(1) # main.go:33:29
    .   .   .   .   NAME-main.~R1 Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:32
    .   .   .   AS2 tc(1) # main.go:33:29
    .   .   .   AS2-Lhs
    .   .   .   .   NAME-main.~R0 Class:PAUTO Offset:0 InlFormal OnStack Used bool tc(1) # main.go:33:29 main.go:11:26
    .   .   .   .   NAME-main.~R1 Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:32
    .   .   .   AS2-Rhs
    .   .   .   .   EQ bool tc(1) # main.go:33:29 main.go:20:14
    .   .   .   .   .   NAME-main.sqrt Class:PAUTO Offset:0 InlLocal OnStack Used float64 tc(1) # main.go:33:29 main.go:12:2
    .   .   .   .   .   CONV float64 tc(1) # main.go:33:29 main.go:20:31
    .   .   .   .   .   .   CONV uint64 tc(1) # main.go:33:29 main.go:20:32
    .   .   .   .   .   .   .   NAME-main.sqrt Class:PAUTO Offset:0 InlLocal OnStack Used float64 tc(1) # main.go:33:29 main.go:12:2
    .   .   .   .   CONV uint64 tc(1) # main.go:33:29 main.go:20:47
    .   .   .   .   .   NAME-main.sqrt Class:PAUTO Offset:0 InlLocal OnStack Used float64 tc(1) # main.go:33:29 main.go:12:2
    .   .   .   GOTO main..i1 tc(1) # main.go:33:29
    .   .   LABEL main..i1 # main.go:33:29
    .   INLCALL-ReturnVars
    .   .   NAME-main.~R0 Class:PAUTO Offset:0 InlFormal OnStack Used bool tc(1) # main.go:33:29 main.go:11:26
    .   .   NAME-main.~R1 Class:PAUTO Offset:0 InlFormal OnStack Used uint64 tc(1) # main.go:33:29 main.go:11:32
    ```

I won't go as far to say as this is a bug in the Go compiler as it might be ignorant of some underlying fact of how the inlined functions are eventually rendered in machine code, but the truth of the matter is I remain confused because of the messages claiming `isSquare` being inlined despite the fact that the compiled code clearly refutes the claim.

## Optimization results

Let's go ahead and use the profile we gathered during our initial run [from the original benchmark](#the-benchmark). 

```
$ go build -pgo=auto -gcflags="-m=3 -pgoprofile=default.pgo -d=pgodebug=1" . &>/dev/null
$ time for i in {1..100} ; do ./fermats-factorization -n 10976191241513578168  ; done
[...]
real    5m37.930s
user    5m4.000s
sys     0m1.198s
```

This totals 3.37930 seconds per run, which is a (1 - (3.37930 / 3.53244)) * 100 = 4.34% improvement. This lies within the 2%-7% range that the Go devs expect can be reasonably achieved, so our optimizations are clearly having an effect!

## Parting thoughts

