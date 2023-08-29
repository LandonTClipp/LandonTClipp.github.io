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
$ go test -cpuprofile default.pgo -bench . -benchtime 20s
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

### Using in-lined pprof

The method used in `main.go` is to use `runtime/pprof`. The code for that is relatively simple:

```go title="profile.go" linenums="1"
--8<-- "code/profile-guided-optimizations/fermats-factorization/profile.go"
```

It starts the CPU profile and returns a `close` function that the caller must `defer close()` in order to stop the profile and close the file.

### Using HTTP pprof

The blog linked above shows us another way to gather profiles. We can instantiate an HTTP server and submit `GET` requests to our program. We do this by starting a server at port 6060 and by importing `net/http/pprof` (which automatically adds handlers to the server on import). The relevant function is `func httpProf()` in our `profile.go` file.

Let's start the program and tell it to infinitely find the factorization:

```
$ ./fermats-factorization -httpprof=true -infinite 1 -cpuprofile=""
```

You can then open a browser to see a simple text-only page at `http://localhost:6060/debug/pprof/`:

```
/debug/pprof/
Set debug=1 as a query parameter to export in legacy text format


Types of profiles available:
Count	Profile
10	allocs
0	block
0	cmdline
4	goroutine
10	heap
0	mutex
0	profile
6	threadcreate
0	trace
full goroutine stack dump
Profile Descriptions:

allocs: A sampling of all past memory allocations
block: Stack traces that led to blocking on synchronization primitives
cmdline: The command line invocation of the current program
goroutine: Stack traces of all current goroutines. Use debug=2 as a query parameter to export in the same format as an unrecovered panic.
heap: A sampling of memory allocations of live objects. You can specify the gc GET parameter to run GC before taking the heap sample.
mutex: Stack traces of holders of contended mutexes
profile: CPU profile. You can specify the duration in the seconds GET parameter. After you get the profile file, use the go tool pprof command to investigate the profile.
threadcreate: Stack traces that led to the creation of new OS threads
trace: A trace of execution of the current program. You can specify the duration in the seconds GET parameter. After you get the trace file, use the go tool trace command to investigate the trace.
```

It shows the links we can get to inspect many things about the program state. Here are some examples of the endpoints you can query:


=== "`goroutine`"

    ``` title=""
    $ curl http://localhost:6060/debug/pprof/goroutine?debug=2
    goroutine 96 [running]:
    runtime/pprof.writeGoroutineStacks({0x76b3e0, 0xc000126000})
            /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:703 +0x6a
    runtime/pprof.writeGoroutine({0x76b3e0?, 0xc000126000?}, 0xc00005c7e8?)
            /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:692 +0x25
    runtime/pprof.(*Profile).WriteTo(0x6c92e0?, {0x76b3e0?, 0xc000126000?}, 0xc?)
            /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:329 +0x146
    net/http/pprof.handler.ServeHTTP({0xc0000203a1, 0x9}, {0x76cf68, 0xc000126000}, 0x76b180?)
            /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:267 +0x4a8
    net/http/pprof.Index({0x76cf68?, 0xc000126000}, 0xc000122000?)
            /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:384 +0xe5
    net/http.HandlerFunc.ServeHTTP(0x443d60?, {0x76cf68?, 0xc000126000?}, 0x61a0fa?)
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2136 +0x29
    net/http.(*ServeMux).ServeHTTP(0x95d4e0?, {0x76cf68, 0xc000126000}, 0xc000122000)
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2514 +0x142
    net/http.serverHandler.ServeHTTP({0xc000089140?}, {0x76cf68?, 0xc000126000?}, 0x6?)
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2938 +0x8e
    net/http.(*conn).serve(0xc0001e0090, {0x76d5d0, 0xc0000890e0})
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2009 +0x5f4
    created by net/http.(*Server).Serve in goroutine 6
            /home/lclipp/src/golang-1.21/src/net/http/server.go:3086 +0x5cb

    goroutine 1 [runnable]:
    sync.(*Mutex).Unlock(...)
            /home/lclipp/src/golang-1.21/src/sync/mutex.go:219
    sync.(*RWMutex).Unlock(0x95da90)
            /home/lclipp/src/golang-1.21/src/sync/rwmutex.go:216 +0x8e
    syscall.Setenv({0x6f054a, 0xf}, {0x7031ee, 0x2})
            /home/lclipp/src/golang-1.21/src/syscall/env_unix.go:123 +0x36a
    os.Setenv({0x6f054a?, 0x0?}, {0x7031ee?, 0x283?})
            /home/lclipp/src/golang-1.21/src/os/env.go:120 +0x25
    main.isSquare(0x4018000000000000?)
            /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:18 +0x6a
    main.findFactors(0x1f73)
            /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:33 +0xbb
    main.main()
            /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:61 +0x273

    goroutine 6 [IO wait]:
    internal/poll.runtime_pollWait(0x7ffff7e47820, 0x72)
            /home/lclipp/src/golang-1.21/src/runtime/netpoll.go:343 +0x85
    internal/poll.(*pollDesc).wait(0xc0000ae100?, 0x5?, 0x0)
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_poll_runtime.go:84 +0x27
    internal/poll.(*pollDesc).waitRead(...)
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_poll_runtime.go:89
    internal/poll.(*FD).Accept(0xc0000ae100)
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_unix.go:611 +0x2ac
    net.(*netFD).accept(0xc0000ae100)
            /home/lclipp/src/golang-1.21/src/net/fd_unix.go:172 +0x29
    net.(*TCPListener).accept(0xc00006a1e0)
            /home/lclipp/src/golang-1.21/src/net/tcpsock_posix.go:152 +0x1e
    net.(*TCPListener).Accept(0xc00006a1e0)
            /home/lclipp/src/golang-1.21/src/net/tcpsock.go:315 +0x30
    net/http.(*Server).Serve(0xc0001ba000, {0x76d178, 0xc00006a1e0})
            /home/lclipp/src/golang-1.21/src/net/http/server.go:3056 +0x364
    net/http.(*Server).ListenAndServe(0xc0001ba000)
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2985 +0x71
    net/http.ListenAndServe(...)
            /home/lclipp/src/golang-1.21/src/net/http/server.go:3239
    main.httpProf.func1()
            /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/profile.go:35 +0x3a
    created by main.httpProf in goroutine 1
            /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/profile.go:34 +0x1a

    goroutine 29 [IO wait]:
    internal/poll.runtime_pollWait(0x7ffff7e47728, 0x72)
            /home/lclipp/src/golang-1.21/src/runtime/netpoll.go:343 +0x85
    internal/poll.(*pollDesc).wait(0xc0000ae000?, 0xc0001e2000?, 0x0)
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_poll_runtime.go:84 +0x27
    internal/poll.(*pollDesc).waitRead(...)
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_poll_runtime.go:89
    internal/poll.(*FD).Read(0xc0000ae000, {0xc0001e2000, 0x1000, 0x1000})
            /home/lclipp/src/golang-1.21/src/internal/poll/fd_unix.go:164 +0x27a
    net.(*netFD).Read(0xc0000ae000, {0xc0001e2000?, 0x4b3d45?, 0x0?})
            /home/lclipp/src/golang-1.21/src/net/fd_posix.go:55 +0x25
    net.(*conn).Read(0xc000038008, {0xc0001e2000?, 0x0?, 0xc000088e78?})
            /home/lclipp/src/golang-1.21/src/net/net.go:179 +0x45
    net/http.(*connReader).Read(0xc000088e70, {0xc0001e2000, 0x1000, 0x1000})
            /home/lclipp/src/golang-1.21/src/net/http/server.go:791 +0x14b
    bufio.(*Reader).fill(0xc00006e1e0)
            /home/lclipp/src/golang-1.21/src/bufio/bufio.go:113 +0x103
    bufio.(*Reader).Peek(0xc00006e1e0, 0x4)
            /home/lclipp/src/golang-1.21/src/bufio/bufio.go:151 +0x53
    net/http.(*conn).serve(0xc0001e0000, {0x76d5d0, 0xc0000890e0})
            /home/lclipp/src/golang-1.21/src/net/http/server.go:2044 +0x75c
    created by net/http.(*Server).Serve in goroutine 6
            /home/lclipp/src/golang-1.21/src/net/http/server.go:3086 +0x5cb

    goroutine 97 [runnable]:
    net/http.(*connReader).startBackgroundRead.func2()
            /home/lclipp/src/golang-1.21/src/net/http/server.go:679
    runtime.goexit()
            /home/lclipp/src/golang-1.21/src/runtime/asm_amd64.s:1650 +0x1
    created by net/http.(*connReader).startBackgroundRead in goroutine 96
            /home/lclipp/src/golang-1.21/src/net/http/server.go:679 +0xba
    ```

=== "`allocs`"

    ``` title=""
    $ curl http://localhost:6060/debug/pprof/allocs?debug=2
    heap profile: 1: 24 [120369: 65498176] @ heap/1048576
    1: 24 [66654: 1599696] @ 0x454bb2 0x4545a5 0x454805 0x49bda5 0x4b82e5 0x67466a 0x6747db 0x674ab3 0x43b5bb 0x46d781
    #       0x49bda4        syscall.Setenv+0x1e4    /home/lclipp/src/golang-1.21/src/syscall/env_unix.go:114
    #       0x4b82e4        os.Setenv+0x24          /home/lclipp/src/golang-1.21/src/os/env.go:120
    #       0x674669        main.isSquare+0x69      /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:18
    #       0x6747da        main.findFactors+0xba   /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:33
    #       0x674ab2        main.main+0x272         /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:61
    #       0x43b5ba        runtime.main+0x2ba      /home/lclipp/src/golang-1.21/src/runtime/proc.go:267

    0: 0 [2: 1327104] @ 0x5e7685 0x5e765f 0x665bab 0x66c245 0x66a74b 0x669826 0x664fe5 0x46d781
    #       0x5e7684        compress/flate.NewWriter+0x2e4                                  /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:666
    #       0x5e765e        compress/gzip.(*Writer).Write+0x2be                             /home/lclipp/src/golang-1.21/src/compress/gzip/gzip.go:191
    #       0x665baa        runtime/pprof.(*profileBuilder).flush+0x4a                      /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:146
    #       0x66c244        runtime/pprof.(*profileBuilder).emitLocation+0x13c4             /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:650
    #       0x66a74a        runtime/pprof.(*profileBuilder).appendLocsForStack+0x4ca        /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:463
    #       0x669825        runtime/pprof.(*profileBuilder).build+0x205                     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:376
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [2: 448] @ 0x668c10 0x668be6 0x66a5f3 0x669826 0x664fe5 0x46d781
    #       0x668be5        runtime/pprof.allFrames+0x25                                    /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:214
    #       0x66a5f2        runtime/pprof.(*profileBuilder).appendLocsForStack+0x372        /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:447
    #       0x669825        runtime/pprof.(*profileBuilder).build+0x205                     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:376
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [1: 262144] @ 0x5ddacb 0x5e769e 0x5e765f 0x669a78 0x664fe5 0x46d781
    #       0x5ddaca        compress/flate.(*compressor).init+0x46a         /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:586
    #       0x5e769d        compress/flate.NewWriter+0x2fd                  /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:667
    #       0x5e765e        compress/gzip.(*Writer).Write+0x2be             /home/lclipp/src/golang-1.21/src/compress/gzip/gzip.go:191
    #       0x669a77        runtime/pprof.(*profileBuilder).build+0x457     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:390
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [1: 663552] @ 0x5e7685 0x5e765f 0x669a78 0x664fe5 0x46d781
    #       0x5e7684        compress/flate.NewWriter+0x2e4                  /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:666
    #       0x5e765e        compress/gzip.(*Writer).Write+0x2be             /home/lclipp/src/golang-1.21/src/compress/gzip/gzip.go:191
    #       0x669a77        runtime/pprof.(*profileBuilder).build+0x457     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:390
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [1: 139264] @ 0x5dda47 0x5dda9d 0x5e769e 0x5e765f 0x665bab 0x66c245 0x66a74b 0x669826 0x664fe5 0x46d781
    #       0x5dda46        compress/flate.newDeflateFast+0x3e6                             /home/lclipp/src/golang-1.21/src/compress/flate/deflatefast.go:64
    #       0x5dda9c        compress/flate.(*compressor).init+0x43c                         /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:585
    #       0x5e769d        compress/flate.NewWriter+0x2fd                                  /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:667
    #       0x5e765e        compress/gzip.(*Writer).Write+0x2be                             /home/lclipp/src/golang-1.21/src/compress/gzip/gzip.go:191
    #       0x665baa        runtime/pprof.(*profileBuilder).flush+0x4a                      /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:146
    #       0x66c244        runtime/pprof.(*profileBuilder).emitLocation+0x13c4             /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:650
    #       0x66a74a        runtime/pprof.(*profileBuilder).appendLocsForStack+0x4ca        /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:463
    #       0x669825        runtime/pprof.(*profileBuilder).build+0x205                     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:376
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [1: 2048] @ 0x61ae87 0x61ae60 0x61c41e 0x620059 0x46d781
    #       0x61ae86        bufio.NewWriterSize+0x1c6               /home/lclipp/src/golang-1.21/src/bufio/bufio.go:599
    #       0x61ae5f        net/http.newBufioWriterSize+0x19f       /home/lclipp/src/golang-1.21/src/net/http/server.go:853
    #       0x61c41d        net/http.(*conn).readRequest+0x9dd      /home/lclipp/src/golang-1.21/src/net/http/server.go:1066
    #       0x620058        net/http.(*conn).serve+0x338            /home/lclipp/src/golang-1.21/src/net/http/server.go:1934

    0: 0 [1: 131072] @ 0x448c59 0x4085f1 0x664d78 0x671885 0x6214e9 0x622e02 0x623aae 0x620314 0x46d781
    #       0x664d77        runtime/pprof.StartCPUProfile+0xf7      /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:793
    #       0x671884        net/http/pprof.Profile+0x2a4            /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:149
    #       0x6214e8        net/http.HandlerFunc.ServeHTTP+0x28     /home/lclipp/src/golang-1.21/src/net/http/server.go:2136
    #       0x622e01        net/http.(*ServeMux).ServeHTTP+0x141    /home/lclipp/src/golang-1.21/src/net/http/server.go:2514
    #       0x623aad        net/http.serverHandler.ServeHTTP+0x8d   /home/lclipp/src/golang-1.21/src/net/http/server.go:2938
    #       0x620313        net/http.(*conn).serve+0x5f3            /home/lclipp/src/golang-1.21/src/net/http/server.go:2009

    0: 0 [53: 55574528] @ 0x6648f0 0x664865 0x661326 0x672908 0x673405 0x6214e9 0x622e02 0x623aae 0x620314 0x46d781
    #       0x6648ef        runtime/pprof.writeGoroutineStacks+0x2f /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:701
    #       0x664864        runtime/pprof.writeGoroutine+0x24       /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:692
    #       0x661325        runtime/pprof.(*Profile).WriteTo+0x145  /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:329
    #       0x672907        net/http/pprof.handler.ServeHTTP+0x4a7  /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:267
    #       0x673404        net/http/pprof.Index+0xe4               /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:384
    #       0x6214e8        net/http.HandlerFunc.ServeHTTP+0x28     /home/lclipp/src/golang-1.21/src/net/http/server.go:2136
    #       0x622e01        net/http.(*ServeMux).ServeHTTP+0x141    /home/lclipp/src/golang-1.21/src/net/http/server.go:2514
    #       0x623aad        net/http.serverHandler.ServeHTTP+0x8d   /home/lclipp/src/golang-1.21/src/net/http/server.go:2938
    #       0x620313        net/http.(*conn).serve+0x5f3            /home/lclipp/src/golang-1.21/src/net/http/server.go:2009

    0: 0 [1: 81920] @ 0x40fb4d 0x41134f 0x412d39 0x66b1ad 0x66a4f6 0x669826 0x664fe5 0x46d781
    #       0x66b1ac        runtime/pprof.(*profileBuilder).emitLocation+0x32c              /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:602
    #       0x66a4f5        runtime/pprof.(*profileBuilder).appendLocsForStack+0x275        /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:431
    #       0x669825        runtime/pprof.(*profileBuilder).build+0x205                     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:376
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [1: 663552] @ 0x5e7685 0x5e765f 0x665bab 0x66c245 0x66a4f6 0x669826 0x664fe5 0x46d781
    #       0x5e7684        compress/flate.NewWriter+0x2e4                                  /home/lclipp/src/golang-1.21/src/compress/flate/deflate.go:666
    #       0x5e765e        compress/gzip.(*Writer).Write+0x2be                             /home/lclipp/src/golang-1.21/src/compress/gzip/gzip.go:191
    #       0x665baa        runtime/pprof.(*profileBuilder).flush+0x4a                      /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:146
    #       0x66c244        runtime/pprof.(*profileBuilder).emitLocation+0x13c4             /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:650
    #       0x66a4f5        runtime/pprof.(*profileBuilder).appendLocsForStack+0x275        /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:431
    #       0x669825        runtime/pprof.(*profileBuilder).build+0x205                     /home/lclipp/src/golang-1.21/src/runtime/pprof/proto.go:376
    #       0x664fe4        runtime/pprof.profileWriter+0xc4                                /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:823

    0: 0 [4: 4194304] @ 0x448c16 0x4085f1 0x664d78 0x671885 0x6214e9 0x622e02 0x623aae 0x620314 0x46d781
    #       0x664d77        runtime/pprof.StartCPUProfile+0xf7      /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:793
    #       0x671884        net/http/pprof.Profile+0x2a4            /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:149
    #       0x6214e8        net/http.HandlerFunc.ServeHTTP+0x28     /home/lclipp/src/golang-1.21/src/net/http/server.go:2136
    #       0x622e01        net/http.(*ServeMux).ServeHTTP+0x141    /home/lclipp/src/golang-1.21/src/net/http/server.go:2514
    #       0x623aad        net/http.serverHandler.ServeHTTP+0x8d   /home/lclipp/src/golang-1.21/src/net/http/server.go:2938
    #       0x620313        net/http.(*conn).serve+0x5f3            /home/lclipp/src/golang-1.21/src/net/http/server.go:2009

    0: 0 [8797: 140752] @ 0x409593 0x40959e 0x46920b 0x49bf12 0x4b82e5 0x67466a 0x6747db 0x674ab3 0x43b5bb 0x46d781
    #       0x46920a        syscall.runtimeSetenv+0x2a      /home/lclipp/src/golang-1.21/src/runtime/runtime.go:130
    #       0x49bf11        syscall.Setenv+0x351            /home/lclipp/src/golang-1.21/src/syscall/env_unix.go:122
    #       0x4b82e4        os.Setenv+0x24                  /home/lclipp/src/golang-1.21/src/os/env.go:120
    #       0x674669        main.isSquare+0x69              /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:18
    #       0x6747da        main.findFactors+0xba           /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:33
    #       0x674ab2        main.main+0x272                 /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:61
    #       0x43b5ba        runtime.main+0x2ba              /home/lclipp/src/golang-1.21/src/runtime/proc.go:267

    0: 0 [44849: 717584] @ 0x409552 0x40959e 0x46920b 0x49bf12 0x4b82e5 0x67466a 0x6747db 0x674ab3 0x43b5bb 0x46d781
    #       0x46920a        syscall.runtimeSetenv+0x2a      /home/lclipp/src/golang-1.21/src/runtime/runtime.go:130
    #       0x49bf11        syscall.Setenv+0x351            /home/lclipp/src/golang-1.21/src/syscall/env_unix.go:122
    #       0x4b82e4        os.Setenv+0x24                  /home/lclipp/src/golang-1.21/src/os/env.go:120
    #       0x674669        main.isSquare+0x69              /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:18
    #       0x6747da        main.findFactors+0xba           /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:33
    #       0x674ab2        main.main+0x272                 /home/lclipp/git/LandonTClipp/LandonTClipp.github.io/code/profile-guided-optimizations/fermats-factorization/main.go:61
    #       0x43b5ba        runtime.main+0x2ba              /home/lclipp/src/golang-1.21/src/runtime/proc.go:267

    0: 0 [1: 208] @ 0x4bf45a 0x485171 0x4bf49e 0x4bf945 0x619131 0x5d19ca 0x61f46a 0x61f1aa 0x664965 0x664865 0x661326 0x672908 0x673405 0x6214e9 0x622e02 0x623aae 0x620314 0x46d781
    #       0x4bf459        fmt.glob..func1+0x19                    /home/lclipp/src/golang-1.21/src/fmt/print.go:147
    #       0x485170        sync.(*Pool).Get+0xb0                   /home/lclipp/src/golang-1.21/src/sync/pool.go:151
    #       0x4bf49d        fmt.newPrinter+0x1d                     /home/lclipp/src/golang-1.21/src/fmt/print.go:152
    #       0x4bf944        fmt.Fprintf+0x44                        /home/lclipp/src/golang-1.21/src/fmt/print.go:223
    #       0x619130        net/http.(*chunkWriter).Write+0xd0      /home/lclipp/src/golang-1.21/src/net/http/server.go:382
    #       0x5d19c9        bufio.(*Writer).Write+0xe9              /home/lclipp/src/golang-1.21/src/bufio/bufio.go:682
    #       0x61f469        net/http.(*response).write+0x229        /home/lclipp/src/golang-1.21/src/net/http/server.go:1648
    #       0x61f1a9        net/http.(*response).Write+0x29         /home/lclipp/src/golang-1.21/src/net/http/server.go:1606
    #       0x664964        runtime/pprof.writeGoroutineStacks+0xa4 /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:714
    #       0x664864        runtime/pprof.writeGoroutine+0x24       /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:692
    #       0x661325        runtime/pprof.(*Profile).WriteTo+0x145  /home/lclipp/src/golang-1.21/src/runtime/pprof/pprof.go:329
    #       0x672907        net/http/pprof.handler.ServeHTTP+0x4a7  /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:267
    #       0x673404        net/http/pprof.Index+0xe4               /home/lclipp/src/golang-1.21/src/net/http/pprof/pprof.go:384
    #       0x6214e8        net/http.HandlerFunc.ServeHTTP+0x28     /home/lclipp/src/golang-1.21/src/net/http/server.go:2136
    #       0x622e01        net/http.(*ServeMux).ServeHTTP+0x141    /home/lclipp/src/golang-1.21/src/net/http/server.go:2514
    #       0x623aad        net/http.serverHandler.ServeHTTP+0x8d   /home/lclipp/src/golang-1.21/src/net/http/server.go:2938
    #       0x620313        net/http.(*conn).serve+0x5f3            /home/lclipp/src/golang-1.21/src/net/http/server.go:2009


    # runtime.MemStats
    # Alloc = 2898784
    # TotalAlloc = 63342021312
    # Sys = 14128392
    # Lookups = 0
    # Mallocs = 4393313221
    # Frees = 4393172386
    # HeapAlloc = 2898784
    # HeapSys = 8060928
    # HeapIdle = 4669440
    # HeapInuse = 3391488
    # HeapReleased = 3219456
    # HeapObjects = 140835
    # Stack = 327680 / 327680
    # MSpan = 78960 / 130368
    # MCache = 1200 / 15600
    # BuckHashSys = 1447666
    # GCSys = 3582536
    # OtherSys = 563614
    # NextGC = 4194304
    # LastGC = 1693254042011951410
    # PauseNs = [15738 15147 18294 17224 18183 17745 19846 17127 16522 19752 18705 16005 16843 15907 18858 16735 16477 15636 15484 16719 18167 17087 21555 14918 19435 16284 16268 19483 16772 17313 17855 20875 20069 18587 19127 16946 26900 17360 18050 16686 18122 19277 20227 19745 17115 16540 15493 18496 18256 15830 19711 19601 16245 14577 20009 15254 13715 18802 19667 13109 13124 13934 15588 14388 18209 18991 14374 15740 14289 20901 17489 21279 19773 28754 21555 30303 17949 19346 18672 18419 19476 20368 18432 19456 20018 16892 18307 18167 20866 17833 22653 19734 19191 19368 21210 19033 22139 21464 19078 20702 16826 20748 17718 21021 24627 47368 14741 15481 23505 14277 14901 13454 11464 13308 9947 11432 12941 11486 11427 10886 8986 10753 10442 12987 12185 10501 10818 13286 19746 15229 17043 17441 17069 16595 17523 15014 13421 12410 14837 15645 16943 12835 15186 13843 11611 12016 10026 12923 18868 18457 14253 14538 12394 13432 11791 10838 9484 11557 8732 12277 12829 12696 13793 11357 13510 13163 13236 12809 10906 12536 10699 11082 12279 9137 13193 9978 12473 10930 11201 11821 11545 12614 12449 13316 14258 10000 11677 13725 10689 9826 11998 9937 11589 9960 9639 10718 12885 13390 17717 14061 14581 14990 15566 16201 16416 14345 18051 17004 15580 15666 14849 13756 12916 12604 15424 13942 11558 12558 12404 13323 12915 13685 13240 13354 16053 13123 15550 13330 13042 13153 15704 14922 16622 12242 12509 12051 11135 11342 8983 11569 11205 10889 18163 11581 12828 1947286 13709 10892 12980 13700 13958 21487 17441 18360 20350 16990]
    # PauseEnd = [1693254032356026598 1693254032434344190 1693254032511523723 1693254032580744959 1693254032658734338 1693254032739452192 1693254032838703547 1693254032915592440 1693254032986466109 1693254033057715755 1693254033131601198 1693254033200276633 1693254033279046899 1693254033359203407 1693254033436649675 1693254033500461415 1693254033580186855 1693254033653540387 1693254033721747647 1693254033782114169 1693254033862576624 1693254033926629869 1693254034006734174 1693254034068732411 1693254034147826819 1693254034207756249 1693254034285648043 1693254034365543898 1693254034430735834 1693254034506017919 1693254034582158839 1693254034650749932 1693254034727687928 1693254034788764427 1693254034871739486 1693254034952732009 1693254035012736393 1693254035069730538 1693254035133734285 1693254035190759959 1693254035250755401 1693254035331752146 1693254035410531611 1693254035489214917 1693254035553743678 1693254035632977911 1693254035708030771 1693254035776772543 1693254035834137128 1693254035878587056 1693254035956237696 1693254036030605817 1693254036065979564 1693254036097003971 1693254036155155766 1693254036192577304 1693254036237859347 1693254036382899771 1693254036539659402 1693254036579768118 1693254036615412063 1693254036649182708 1693254036718827411 1693254036781441392 1693254036903037348 1693254036984405072 1693254037024201893 1693254037069437637 1693254037104662311 1693254037205256113 1693254037426919129 1693254037567766316 1693254037628626055 1693254037709440070 1693254037913713487 1693254038133953584 1693254038214452113 1693254038253161537 1693254038301449662 1693254038355187073 1693254038439016055 1693254038497041412 1693254038533466162 1693254038570716819 1693254038617603722 1693254038648416198 1693254038688622609 1693254038725792731 1693254038761852241 1693254038789381796 1693254038829513811 1693254038869723637 1693254038909961560 1693254038946641780 1693254038981076167 1693254039010714206 1693254039049694000 1693254039084111165 1693254039111441626 1693254039151523229 1693254039185165311 1693254039211920753 1693254039252133346 1693254039286131480 1693254039312546528 1693254039353041425 1693254039377037498 1693254039417270815 1693254039452716868 1693254039477603551 1693254039517813064 1693254039549959214 1693254039578179371 1693254039615376283 1693254039638542497 1693254039678720407 1693254039713446968 1693254039739053325 1693254039778993746 1693254039812903507 1693254039839819226 1693254039877305107 1693254039900183277 1693254039940369744 1693254039974679793 1693254040000850394 1693254040040627081 1693254040073213634 1693254040101349456 1693254040141697270 1693254040177178811 1693254040212754112 1693254040242078487 1693254040282254879 1693254040319618150 1693254040353011987 1693254040383276105 1693254040423027125 1693254040459610131 1693254040483384551 1693254040523587854 1693254040560954603 1693254040597660290 1693254040624136362 1693254040664330925 1693254040698511048 1693254040724745881 1693254040766706246 1693254040800322354 1693254040825316448 1693254040865529006 1693254040903015532 1693254040925857119 1693254040966063670 1693254041003938905 1693254041036829938 1693254041066662480 1693254041104046373 1693254041126962908 1693254041167214757 1693254041206610355 1693254041242271171 1693254041267779503 1693254041308011635 1693254041343830717 1693254041380211158 1693254041408659283 1693254041447474010 1693254041479168472 1693254041521489531 1693254041549451298 1693254041586989505 1693254041620576389 1693254041649989018 1693254041685191913 1693254041710335896 1693254041749210376 1693254041770715879 1693254041810880301 1693254041845216005 1693254041871426172 1693254041911421497 1693254041941906383 1693254041971720900 1693254042011951410 1693254029656657195 1693254029696837290 1693254029731438302 1693254029757175524 1693254029796688326 1693254029828174248 1693254029857710025 1693254029894376039 1693254029918065455 1693254029958283518 1693254029998391468 1693254030038813154 1693254030078908471 1693254030112373391 1693254030139361020 1693254030179566187 1693254030215435878 1693254030239924585 1693254030280148602 1693254030318529558 1693254030355864341 1693254030380759129 1693254030420963274 1693254030461162676 1693254030498149066 1693254030532959926 1693254030561734507 1693254030601920225 1693254030648213404 1693254030683037433 1693254030722723163 1693254030757530021 1693254030783592465 1693254030823844974 1693254030864064641 1693254030897835285 1693254030924446424 1693254030964653818 1693254030999079738 1693254031025038086 1693254031065223466 1693254031101040397 1693254031125663530 1693254031165838255 1693254031200509043 1693254031226192180 1693254031266392838 1693254031301318161 1693254031326801974 1693254031367007899 1693254031407242017 1693254031444303050 1693254031467604280 1693254031507738480 1693254031544660337 1693254031568142921 1693254031608388878 1693254031646354839 1693254031668734796 1693254031708946426 1693254031747765010 1693254031781050463 1693254031809531109 1693254031849690064 1693254031884258829 1693254031930733410 1693254032010639093 1693254032071034570 1693254032151730742 1693254032211874819 1693254032292307352]
    # NumGC = 20665
    # NumForcedGC = 0
    # GCCPUFraction = 0.004821695679555479
    # DebugGC = false
    # MaxRSS = 12476416
    ```

=== "`threadcreate`"

    ``` title=""
    $ curl http://localhost:6060/debug/pprof/threadcreate?debug=1
    threadcreate profile: total 6
    5 @
    #       0x0

    1 @ 0x43e985 0x43f2f5 0x43f5e5 0x43b513 0x46d781
    #       0x43e984        runtime.allocm+0xc4                     /home/lclipp/src/golang-1.21/src/runtime/proc.go:1935
    #       0x43f2f4        runtime.newm+0x34                       /home/lclipp/src/golang-1.21/src/runtime/proc.go:2398
    #       0x43f5e4        runtime.startTemplateThread+0x84        /home/lclipp/src/golang-1.21/src/runtime/proc.go:2475
    #       0x43b512        runtime.main+0x212                      /home/lclipp/src/golang-1.21/src/runtime/proc.go:239

    ```

The method to actually get a profile can be done using the `profile` endpoint. By default, hitting the endpoint will collect an endpoint over a period of 30 seconds, but this can be modified through the `seconds` query parameter. For example:

``` title=""
$ curl -o /dev/null -v http://localhost:6060/debug/pprof/profile?seconds=1
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0* About to connect() to localhost port 6060 (#0)
*   Trying 127.0.0.1...
* Connected to localhost (127.0.0.1) port 6060 (#0)
> GET /debug/pprof/profile?seconds=1 HTTP/1.1
> User-Agent: curl/7.29.0
> Host: localhost:6060
> Accept: */*
> 
  0     0    0     0    0     0      0      0 --:--:--  0:00:01 --:--:--     0< HTTP/1.1 200 OK
< Content-Disposition: attachment; filename="profile"
< Content-Type: application/octet-stream
< X-Content-Type-Options: nosniff
< Date: Mon, 28 Aug 2023 20:25:04 GMT
< Transfer-Encoding: chunked
< 
{ [data not shown]
100  3054    0  3054    0     0   2613      0 --:--:--  0:00:01 --:--:--  2625
* Connection #0 to host localhost left intact
```


## What gets optimized and why?

The Go compiler has this concept called an "inlining budget". The budget controls the maximum number of syntatical nodes that a function can have before its considered not inlinable. A node is roughly analagous to the number of nodes in the program's Abstract Syntax Tree (AST), or rather each individual element of a piece of code, like a name declaration, an assignment, a constant declaration, function calls etc. By default, the [`inlineMaxBudget`](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L46) has a value of 80, which means that any functions with more than 80 nodes are not inlinable. If you have profiled your program and the profiler has determined your particular function is "hot", then the [budget increases to 2000](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L76).

### `pgoinlinebudget`

You can modify the inline budget set for hot functions using the `pgoinlinebudget` flag, for example:

```
$ go build -gcflags="-d=pgoinlinebudget=2000" .
```

This is set to 2000 by default but you can specify any value you want. This name is a bit confusing because this does not control the "non-hot" budget, which appears it can't be changed. It _only_ controls the budget for hot functions.

### `pgoinlinecdfthreshold`

The way to read this variable is "Profile Guided Optimization Inline Cumulative Distribution Function Threshold". Wow, what a mouthful! Simply put, this threshold sets the lower bound that the weight of a function must have in order to be considered hot. Let's take a look at what this means in practice. We'll set a `pgoinlinecdfthreshold=90` and run the PGO build and graph the DOT notation conveniently provided to us. Note that we've already generated _one_ `default.pgo` profile by simply running the program without any optimizations applied.

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

Indeed if we sum the weights of the two red edges and divide by the total weight, we get:

$$
\frac{3.92 + 1.86}{3.92 + 1.86 + 1.03 + 0.62 + 0.41 + 0.41 + 0.21} * 100 = 68.3%
$$

If we were to include the next highest weight of 1.03, the CDF would give us 80.5%, so it appears Go's algoritm is exclusive in its calculations. Or in other words, it keeps the cumulative distribution at or below the threshold you define.

### What is a CDF?

[Cumulative Distribution Functions](https://en.wikipedia.org/wiki/Cumulative_distribution_function) are mathematical models that tell you the probability `y` that some random variable `X` will take on a value less than or equal to the value on the `x` axis. In statistics, this can be used, for example, to find the probability that you will draw from a deck of cards the value between 2-8 (aces high). The CDF for such a scenaio is quite simple, since the probability of drawing any particular valued card is uniformly distributed at 1/13. Thus the CDF is:

$$
F_X(x) = \begin{cases}
\frac{x-1}{13} &:\ 2 <= x <= 14
\end{cases}
$$

For the purposes of determining function hotness, we're looking at a CDF from a slightly different perspective. We're asking the question: "given a certain percentage $p$ (that being percentage of runtime), what is the edge weight threshold $F(p)$ such that the sum of all edge weights at or above $F(p)$ equals $p$ percentage of the total program runtime?" The answer $F(p)$ is the `hot-callsite-thres-from-CDF` value we saw Go print out, and $p$ is the `pgoinlinecdfthreshold` value we specified to the build process. We can mathematically describe our situation:


$$
W = \{w_0, w_1, ... w_n\}
$$

$$
w_i > w_{i+1}
$$

Where $W$ is the set of all edge weights in a program.

$$
F(p) = \frac{(\sum_{j=0}^{m} W_i) \le x}{\sum W}
$$

Where $p$ is the `pgoinlinecdfthreshold` passed to the build. Basically, we sum the weights $W$ up to index $m$ such that the sum is less than or equal to our `pgoinlinecdfthreshold` value. $F(p)$ is the resultant `hot-callsite-thres-from-CDF` value that determines the lowest weight an edge must have in order for it to be considered hot. Go's implementation of this equation is [here](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L122-L155).

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

I won't go as far to say as this is a bug in the Go compiler as I might be ignorant of some underlying fact of how the inlined functions are eventually rendered in machine code, but the truth of the matter is I remain confused because of the messages claiming `isSquare` being inlined despite compiled code clearly refuting this.

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

