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

The type of workload we're aiming to optimize is a compute-heavy one. Therefore we want to create a program that performs some computationally heavy algorithm. One of the hardest computational problems is the factorization of the product of two large prime numbers, a problem that RSA cryptography relies on being exceedingly expensive for its security guarantees. There are [a large number of algorithms](https://en.wikipedia.org/wiki/Integer_factorization#Factoring_algorithms) we could choose from, but for no particular reason at all (well, perhaps because of its relative simplicity), I'll choose the [Fermant's Factorization](https://en.wikipedia.org/wiki/Fermat%27s_factorization_method).

```go title="main.go"
--8<-- "code/profile-guided-optimizations/fermants-factorization/main.go"
```

We can write a simple benchmarking test for `findFactors`

```go title="main_test.go"
--8<-- "code/profile-guided-optimizations/fermants-factorization/main_test.go"
```

And then run the benchmark for 20 seconds and get an average result

```bash
$ go test -bench=. -benchtime 20s


numIterations=1377437 fact1=3314411264 fact2=3311656390

goos: linux
goarch: amd64
pkg: fermants-factorization
cpu: Intel(R) Xeon(R) CPU E5-2699A v4 @ 2.40GHz
BenchmarkFindFactors 

numIterations=1377437 fact1=3314411264 fact2=3311656390



numIterations=1377437 fact1=3314411264 fact2=3311656390

     681          32507910 ns/op
PASS
ok      fermants-factorization  25.699s
```

This is useful, but we want to find the average execution time of the executable, not the test. So instead we can run it 100 times and average the result:

```bash
$ time for i in {1..100} ; do ./fermants-factorization -n 10976191241513578168 ; done
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
[...]

real    5m40.590s
user    5m17.049s
sys     0m0.564s
```

This gives us an average runtime of 3.4059 seconds.

## Gathering Profiles

### Using test-based pprof

We can generate a CPU profile by simply enabling it during tests. This will give us a rather artificial example of the CPU profile as compared to its real-world behavior, but it might be interesting nonetheless. Let's see what happens.


We run the test benchmarking and tell `go test` to output the cpuprofile to a file named `default.pgo`, which is the default name that `go build` looks for when reading profiles.

```bash
$ go test -cpuprofile default.pgo -memprofile mem.prof -bench . -benchtime 20s
numIterations=1377437 fact1=3314411264 fact2=3311656390
goos: linux
goarch: amd64
pkg: fermants-factorization
cpu: Intel(R) Xeon(R) CPU E5-2699A v4 @ 2.40GHz
BenchmarkFindFactors    numIterations=1377437 fact1=3314411264 fact2=3311656390
       7        3404269889 ns/op
PASS
ok      fermants-factorization  27.268s
```

We can then build our application again and run our command-line profiling to see how the runtime has changed:

```bash
$ go build -pgo=auto .
$ time for i in {1..100} ; do ./fermants-factorization -n 10976191241513578168 ; done
Found factors with i=1377437: 10976191241513578168 = 3314411264 x 3311656390
[...]

real    5m32.447s
user    5m18.394s
sys     0m0.415s
```

This is a total of 3.32447 seconds on average, which translates to a 2.39% speedup.

### Using in-lined pprof

foobar

### Using HTTP pprof

foobar

## What was optimized

foobar

### Dissecting the original binary

By disassembling the original binary, we can see that the call to `isSquare` is not inlined. This is by design actually, as we added an "expensive" call to `NewExpensive()` that caused `isSquare`'s total node count to exceed Go's budget of 80. You can see the compiler tell us this by using verbose GC flags:

```bash
$ go build -pgo=off -gcflags="-m -m -m" . |& grep isSquare
./main.go:11:6: cannot inline isSquare: function too complex: cost 84 exceeds budget 80
```

By doing an objdump, we can also see this fact in the assembly code:

```bash
$ go tool objdump ./fermants-factorization |& grep 'main.go:30'
  main.go:30            0x4887f1                e82afeffff              CALL main.isSquare(SB)       
```

Keep this fact in mind for the following section.

### Dissecting the profile-guided binary

Using the results from the test-driven profile, we can disassemble the executable and look at what optimization decisions the go compiler made. Let's take a look at the line where we call out to `isSquare`:

```go
issquare, sqrt := isSquare(uint64(potentialSquare))
```

```bash
$ go tool objdump ./fermants-factorization
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