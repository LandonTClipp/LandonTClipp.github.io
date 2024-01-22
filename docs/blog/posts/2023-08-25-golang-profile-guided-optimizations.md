---
date: 2023-08-25
categories:
  - Programming
  - Golang
title: Profile Guided Optimizations in Go
description: How to use PGO to improve the performance of your production applications
---

Profile Guided Optimizations in Go
==================================

![A view of the mountains from the top of Pikes Peak](https://sasgidotxvcxfexkslru.supabase.co/storage/v1/object/public/assets/images/banners/pikes-peak-1.jpg?blog=Profile%20Guided%20Optimizations%20in%20Go "A view of the mountains from the top of Pikes Peak. This road, just outside of Woodland Park, CO, travels up an enormous distance, through many hairpin turns and past sheer drops is a hair-raising experience. The top, however, is a breathtaking view that puts you on your heels. This particular view, looking south, is not even at the summit! Oh how I wish I could stay here forever."){ style="width: 100%; height: 200px; object-fit: cover; object-position: 0 40%" }

In this post, we'll explore Profile Guided Optimizations (PGO) introduced in Go 1.20 and how they can be effectively used to improve the performance of your production applications. PGO is a compiler optimization technique that allows you to tune the performance of your Go applications in an environment- and workload-specific way. The profiles themselves are simple metadata files that contain information on what functions are called, how often they're called, what system calls are used, and generally what the computational _profile_ is of your application. This information can be used by the compiler to better inform what sorts of optimizations are useful in your specific environment and workfload. 

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

The type of workload we're aiming to optimize is a compute-heavy one. We will be implementing a simple factorization algorithm, with a sprinkle of some intentional inefficiencies so we can simulate a program with really hot code paths. There are [a large number of algorithms](https://en.wikipedia.org/wiki/Integer_factorization#Factoring_algorithms) we could choose from, but I'll choose [Fermat's Factorization](https://en.wikipedia.org/wiki/Fermat%27s_factorization_method) due to its simplicity.

```go title="main.go" linenums="1"
--8<-- "code/profile-guided-optimizations/fermats-factorization/main.go"
```

<div class="result">
```title=""
$ go build .
$ ./fermats-factorization -n 179957108976619
starting CPU profile
Found factors with i=42698929: 179957108976619 = 1627093 x 110600383
```
</div>

## The Benchmark

The benchmark we'll be using is going to be captured by timing the program's runtime over 60 seconds without any optimizations applied. This will give us a baseline performance value which we can compare to optimized builds later on. This will be done by using the benchmarking test as shown here:

```go title="main_test.go"
--8<-- "code/profile-guided-optimizations/fermats-factorization/main_test.go"
```

We then run the benchmark with `-cpuprofile default.pgo` to tell it to create a CPU profile during the benchmark, and `-pgo=off` to disable PGO (we want an unoptimized run to compare against).

```
$ go test -bench . -benchtime 60s -cpuprofile default.pgo -pgo=off
numIterations=42698929 fact1=1627093 fact2=110600383
goos: linux
goarch: arm64
pkg: fermats-factorization
BenchmarkFindFactors 	numIterations=42698929 fact1=1627093 fact2=110600383
       9	7173565614 ns/op
PASS
ok  	fermats-factorization	71.860s
```

Our baseline in this benchmark is thus 7173565614 ns/op.


## PGO Result

We can re-run our benchmark, but tell it to use the `default.pgo` CPU profile we generated during the initial benchmark run:

```
$ go test -bench . -benchtime 60s -pgo=auto
numIterations=42698929 fact1=1627093 fact2=110600383
goos: linux
goarch: arm64
pkg: fermats-factorization
BenchmarkFindFactors 	numIterations=42698929 fact1=1627093 fact2=110600383
       9	6702628537 ns/op
PASS
ok  	fermats-factorization	67.028s
```

Our runtime per iteration is 6702628537 ns, which is a full $(1 - \frac{6702628537}{7173565614}) * 100\% = 6.564895371\%$ faster than our original run! That's quite a significant improvement.

## Methods of Profiling

### Using test-based pprof

We can generate a CPU profile by simply enabling it during tests. This will give us a rather artificial example of the CPU profile as compared to its real-world behavior, but it might be interesting nonetheless. Let's see what happens.

We can write a simple benchmarking test for `findFactors`:

=== "`main_test.go`"

    ```go title="main_test.go"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/main_test.go"
    ```

=== "`main.go`"

    ```go title="main.go" linenums="1"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/main.go"
    ```

Recall the last section that we can the benchmark with the parameter `-cpuprofile default.pgo`. As mentioned before, this causes the benchmark to profile the code as its running, and outputs the `default.pgo` profile. 

### Using in-lined pprof

=== "`runtime_profile.go`"

    The method used in `main.go` is to use `runtime/pprof`. The code for that is relatively simple:

    ```go title="runtime_profile.go" linenums="1"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/runtime_profile.go"
    ```

=== "`main.go`"

    ```go title="main.go" linenums="1"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/main.go"
    ```

It starts the CPU profile and returns a `close` function that the caller must `defer close()` in order to stop the profile and close the file.

### Using HTTP pprof

The [blog post previously linked]((https://go.dev/doc/pgo)) shows us another way to gather profiles. We can instantiate an HTTP server and submit `GET` requests to our program. We do this by starting a server at port 6060 and by importing `net/http/pprof` (which automatically adds handlers to the server on import). The relevant function is `func httpProf()` in our `http_profile.go` file.

=== "`http_profile.go`"

    ```go title="http_profile.go" linenums="1"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/http_profile.go"
    ```

=== "`main.go`"

    ```go title="main.go" linenums="1"
    --8<-- "code/profile-guided-optimizations/fermats-factorization/main.go"
    ```

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

The Go compiler has this concept called an "inlining budget". The budget controls the maximum number of syntatical nodes that a function can have before it's considered not inlinable. A node is roughly analagous to the number of nodes in the program's Abstract Syntax Tree (AST), or rather each individual element of a piece of code, like a name declaration, an assignment, a constant declaration, function calls etc. By default, the [`inlineMaxBudget`](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L46) has a value of 80, which means that any functions with more than 80 nodes are not inlinable. If you have profiled your program and the profiler has determined your particular function is "hot", then the [budget increases to 2000](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L76).

### Inlining

[Inlining](https://en.wikipedia.org/wiki/Inline_expansion) is a technique whereby a function call is replaced with, more or less, a copy-paste of the function's code into the calling function's frame. The alternative to inlining, which is just a normal function call, actually performs an enormous amount of work:

1. Save the values of its working registers on the stack
2. Prepare space on the stack for the called function, which could include space for the function to store its return values
3. Push any functions arguments into registers (or on the stack if your arguments don't fit on registers)
4. `CALL` the function
5. Restore the register values
6. Retrieve the return values from the stack (or registers)
7. Perform any ancillary cleanup work like cleaning up stack pointers

Function calls are surprisingly complex, so if we can replace all of that work by pretending that the called function's code was inside of the caller's code, we can get some pretty significant speedups by skipping all of these bookkeeping tasks.


#### [`pgoinlinebudget`](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L100)

You can modify the inline budget set for hot functions using the `pgoinlinebudget` flag, for example:

```
$ go build -gcflags="-d=pgoinlinebudget=2000" .
```

This is set to 2000 by default but you can specify any value you want. This name is a bit confusing because this does not control the "non-hot" budget, which appears can't be changed. It _only_ controls the budget for functions that are considered hot.

#### [`pgoinlinecdfthreshold`](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L87)

The way to read this variable is "Profile Guided Optimization Inline Cumulative Distribution Function Threshold". Wow, what a mouthful! Simply put, this threshold sets the lower bound that the weight of a function must have in order to be considered hot. Or in other words, you can think of it as a percentage of the total runtime, whereby the functions whose edge weights represent the top 95% of total edge weights will be considered hot.

Let's take a look at what this means in practice. We'll set a `pgoinlinecdfthreshold=95` and run the PGO build and graph the [DOT notation](https://graphviz.org/doc/info/lang.html)(1) conveniently provided to us. Note that we've already generated _one_ `default.pgo` profile by simply running the program without any optimizations applied. 
{ .annotate }

1. A DOT graph is simply a way you can specify directed graphs in a text format. This text can be rendered into a graph.

##### =95

Let's build the program with PGO enabled, and set `pgoinlinecdfthreshold=95`. Before we do that, we should generate a new `default.pgo` profile using the inlined profiler to give us a more accurate representation of how the code runs under `main()` (it might not actually be all that different but it's good to be thorough). Remember, we generate `default.go` by simply running our program:

```
$ ./fermats-factorization -n 179957108976619
starting CPU profile
Found factors with i=42698929: 179957108976619 = 1627093 x 110600383
$ go build -pgo=auto -gcflags="-d=pgoinlinecdfthreshold=95,pgodebug=3" .
```

The build command outputs a graph in DOT notation. You can copy-paste the code in https://dreampuf.github.io/GraphvizOnline to create a diagram.

=== "DOT graph visualization"
    ![](/images/golang-profile-guided-optimizations/graphviz-threshold-95.svg)

=== "DOT graph code"
    ```title=""
    digraph G {
        forcelabels=true;
        "flag.Uint64" [color=black, style=solid, label="flag.Uint64,inl_cost=63"];
        "main.runtimeProf.func1" [color=black, style=solid, label="main.runtimeProf.func1"];
        "fmt.Sprintf" [color=black, style=solid, label="fmt.Sprintf"];
        "os.Create" [color=black, style=solid, label="os.Create,inl_cost=72"];
        "flag.Parse" [color=black, style=solid, label="flag.Parse,inl_cost=62"];
        "runtime/pprof.StopCPUProfile" [color=black, style=solid, label="runtime/pprof.StopCPUProfile"];
        "math.Sqrt" [color=black, style=solid, label="math.Sqrt,inl_cost=4"];
        "main.NewExpensive" [color=black, style=solid, label="main.NewExpensive"];
        "main.runtimeProf" [color=black, style=solid, label="main.runtimeProf"];
        "log.Fatal" [color=black, style=solid, label="log.Fatal"];
        "main.main" [color=black, style=solid, label="main.main"];
        "flag.Int" [color=black, style=solid, label="flag.Int,inl_cost=63"];
        "os.Setenv" [color=black, style=solid, label="os.Setenv,inl_cost=90"];
        "main.findFactors" [color=black, style=solid, label="main.findFactors"];
        "fmt.Printf" [color=black, style=solid, label="fmt.Printf,inl_cost=73"];
        "flag.String" [color=black, style=solid, label="flag.String,inl_cost=63"];
        "strconv.Itoa" [color=black, style=solid, label="strconv.Itoa,inl_cost=117"];
        "main.httpProf.func1" [color=black, style=solid, label="main.httpProf.func1"];
        "net/http.ListenAndServe" [color=black, style=solid, label="net/http.ListenAndServe,inl_cost=70"];
        "flag.Bool" [color=black, style=solid, label="flag.Bool,inl_cost=63"];
        "math.Ceil" [color=black, style=solid, label="math.Ceil,inl_cost=61"];
        "main.runtimeProf.func2" [color=black, style=solid, label="main.runtimeProf.func2"];
        "os.(*File).Close" [color=black, style=solid, label="os.(*File).Close,inl_cost=67"];
        "main.isSquare" [color=black, style=solid, label="main.isSquare"];
        "fmt.Println" [color=black, style=solid, label="fmt.Println,inl_cost=72"];
        "main.httpProf" [color=black, style=solid, label="main.httpProf"];
        "log.Println" [color=black, style=solid, label="log.Println,inl_cost=77"];
        "runtime/pprof.StartCPUProfile" [color=black, style=solid, label="runtime/pprof.StartCPUProfile"];
        edge [color=red, style=solid];
        "main.isSquare" -> "math.Sqrt" [label="0.21"];
        edge [color=black, style=solid];
        "main.isSquare" -> "main.NewExpensive" [label="0.07"];
        edge [color=black, style=solid];
        "main.isSquare" -> "os.Setenv" [label="0.15"];
        edge [color=red, style=solid];
        "main.isSquare" -> "strconv.Itoa" [label="0.24"];
        edge [color=red, style=solid];
        "main.findFactors" -> "main.isSquare" [label="0.27"];
        edge [color=black, style=solid];
        "main.findFactors" -> "math.Sqrt" [label="0.00"];
        edge [color=black, style=solid];
        "main.findFactors" -> "math.Ceil" [label="0.00"];
        edge [color=black, style=solid];
        "main.findFactors" -> "math.Sqrt" [label="0.00"];
        edge [color=red, style=solid];
        "main.findFactors" -> "main.isSquare" [label="0.23"];
        edge [color=black, style=solid];
        "main.runtimeProf" -> "fmt.Println" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf" -> "os.Create" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf" -> "log.Fatal" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf" -> "runtime/pprof.StartCPUProfile" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf" -> "log.Fatal" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf.func2" -> "os.(*File).Close" [label="0.00"];
        edge [color=black, style=solid];
        "main.runtimeProf.func2" -> "runtime/pprof.StopCPUProfile" [label="0.00"];
        edge [color=black, style=solid];
        "main.httpProf" -> "main.httpProf.func1" [label="0.00"];
        edge [color=black, style=solid];
        "main.httpProf.func1" -> "log.Println" [label="0.00"];
        edge [color=black, style=solid];
        "main.httpProf.func1" -> "net/http.ListenAndServe" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Uint64" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Parse" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "main.runtimeProf" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "main.findFactors" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "fmt.Sprintf" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.String" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Bool" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "flag.Int" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "main.httpProf" [label="0.00"];
        edge [color=black, style=solid];
        "main.main" -> "main.findFactors" [label="0.12"];
        edge [color=black, style=solid];
        "main.main" -> "fmt.Printf" [label="0.00"];
    }
    ```

You can see here that the PGO determined the path in red is considered hot because its weight exceeds the calculated hot callsite threshold:

```
$ go build -pgo=auto -gcflags="-d=pgoinlinecdfthreshold=95,pgodebug=3" . |& grep hot-callsite-thres-from-CDF
hot-callsite-thres-from-CDF=0.18328445747800587
```

One of the things you might notice is that the profiler is smart enough to distinguish between different calls to the same function. You'll see there are two lines going from `main.findFactors` to `main.isSquare`, because there are two separate `CALL` instructions (one within the for loop, and another bare call).

##### =80

If we decrease the pgoinlinecdfthreshold value to something like 80, we see a dramatically different result:

```
$ go build -pgo=auto -gcflags="-d=pgoinlinecdfthreshold=80,pgodebug=3" . |& grep hot-callsite-thres-from-CDF
hot-callsite-thres-from-CDF=1.935483870967742
```

=== "DOT graph visualization"
    And the visualization shows us that none of the paths are considered hot(1) because none of them are above a weight of 1.935483870967742: 
    { .annotate}

    1. To be clear, and to prevent confusion, there _are indeed_ some nodes that are still being marked as hot, but these nodes all live within the Go runtime, and thus are not being included in this visualization. Setting lower threshold values will still _guarantee_ at least one node is marked as hot.

    ![](/images/golang-profile-guided-optimizations/graphviz-threshold-80.svg)

    

=== "DOT graph code"
    ```
        digraph G {
                forcelabels=true;
                "log.Println" [color=black, style=solid, label="log.Println,inl_cost=77"];
                "flag.String" [color=black, style=solid, label="flag.String,inl_cost=63"];
                "flag.Bool" [color=black, style=solid, label="flag.Bool,inl_cost=63"];
                "main.runtimeProf.func2" [color=black, style=solid, label="main.runtimeProf.func2"];
                "main.runtimeProf.func1" [color=black, style=solid, label="main.runtimeProf.func1"];
                "os.(*File).Close" [color=black, style=solid, label="os.(*File).Close,inl_cost=67"];
                "os.Setenv" [color=black, style=solid, label="os.Setenv,inl_cost=90"];
                "fmt.Println" [color=black, style=solid, label="fmt.Println,inl_cost=72"];
                "fmt.Printf" [color=black, style=solid, label="fmt.Printf,inl_cost=73"];
                "flag.Int" [color=black, style=solid, label="flag.Int,inl_cost=63"];
                "runtime/pprof.StopCPUProfile" [color=black, style=solid, label="runtime/pprof.StopCPUProfile"];
                "main.findFactors" [color=black, style=solid, label="main.findFactors"];
                "fmt.Sprintf" [color=black, style=solid, label="fmt.Sprintf"];
                "math.Sqrt" [color=black, style=solid, label="math.Sqrt,inl_cost=4"];
                "log.Fatal" [color=black, style=solid, label="log.Fatal"];
                "main.main" [color=black, style=solid, label="main.main"];
                "main.httpProf" [color=black, style=solid, label="main.httpProf"];
                "main.NewExpensive" [color=black, style=solid, label="main.NewExpensive"];
                "os.Create" [color=black, style=solid, label="os.Create,inl_cost=72"];
                "main.httpProf.func1" [color=black, style=solid, label="main.httpProf.func1"];
                "main.runtimeProf" [color=black, style=solid, label="main.runtimeProf"];
                "net/http.ListenAndServe" [color=black, style=solid, label="net/http.ListenAndServe,inl_cost=70"];
                "flag.Uint64" [color=black, style=solid, label="flag.Uint64,inl_cost=63"];
                "math.Ceil" [color=black, style=solid, label="math.Ceil,inl_cost=61"];
                "main.isSquare" [color=black, style=solid, label="main.isSquare"];
                "strconv.Itoa" [color=black, style=solid, label="strconv.Itoa,inl_cost=117"];
                "runtime/pprof.StartCPUProfile" [color=black, style=solid, label="runtime/pprof.StartCPUProfile"];
                "flag.Parse" [color=black, style=solid, label="flag.Parse,inl_cost=62"];
                edge [color=black, style=solid];
                "main.isSquare" -> "math.Sqrt" [label="0.21"];
                edge [color=black, style=solid];
                "main.isSquare" -> "main.NewExpensive" [label="0.07"];
                edge [color=black, style=solid];
                "main.isSquare" -> "os.Setenv" [label="0.15"];
                edge [color=black, style=solid];
                "main.isSquare" -> "strconv.Itoa" [label="0.24"];
                edge [color=black, style=solid];
                "main.findFactors" -> "math.Sqrt" [label="0.00"];
                edge [color=black, style=solid];
                "main.findFactors" -> "main.isSquare" [label="0.23"];
                edge [color=black, style=solid];
                "main.findFactors" -> "main.isSquare" [label="0.27"];
                edge [color=black, style=solid];
                "main.findFactors" -> "math.Sqrt" [label="0.00"];
                edge [color=black, style=solid];
                "main.findFactors" -> "math.Ceil" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf" -> "fmt.Println" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf" -> "os.Create" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf" -> "log.Fatal" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf" -> "runtime/pprof.StartCPUProfile" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf" -> "log.Fatal" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf.func2" -> "os.(*File).Close" [label="0.00"];
                edge [color=black, style=solid];
                "main.runtimeProf.func2" -> "runtime/pprof.StopCPUProfile" [label="0.00"];
                edge [color=black, style=solid];
                "main.httpProf" -> "main.httpProf.func1" [label="0.00"];
                edge [color=black, style=solid];
                "main.httpProf.func1" -> "log.Println" [label="0.00"];
                edge [color=black, style=solid];
                "main.httpProf.func1" -> "net/http.ListenAndServe" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "flag.String" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "flag.Parse" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "main.runtimeProf" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "main.findFactors" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "main.findFactors" [label="0.12"];
                edge [color=black, style=solid];
                "main.main" -> "fmt.Printf" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "flag.Uint64" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "flag.Int" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "main.httpProf" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "fmt.Sprintf" [label="0.00"];
                edge [color=black, style=solid];
                "main.main" -> "flag.Bool" [label="0.00"];
        }
    ```

!!! question
    You might be wondering why a lower CDF threshold results in less functions being marked as hot. You have to remember that the CDF itself is derived from a [sorted list of edge weights in descending order](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L135-L148). The compiler [iterates over this sorted list](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L150-L159) and keeps track of the [cumulative sum](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L152). It generates a [cumulative percentage](https://github.com/golang/go/blob/go1.21/src/cmd/compile/internal/inline/inl.go#L153) (which is quite literally what the CDF is) and compares that to the target threshold that we specified. If the CDF is greater than the threshold specified, it returns the edge weight of the node that caused us to go over the CDF threshold (which becomes the `hot-callsite-thres-from-CDF` value) and the list of nodes up to that point.

    If we specify a CDF threshold of 50%, but the largest node in our sorted list is, say, 90% of the CDF, then the only node that will be considered hot would be that single large node. You'll notice that our `80%` case above shows no node being considered hot, but this is probably not true. I'm guessing that there is some node (possibly inside of the Go runtime) that is being marked hot, while none of our user code is.

    As you'll see farther down in [Proving the CDF experimentally](#proving-the-cdf-experimentally), we'll modify the Go compiler so it tells us what node exceeded the threshold!

#### What is a CDF?

[Cumulative Distribution Functions](https://en.wikipedia.org/wiki/Cumulative_distribution_function) are mathematical models that tell you the probability `y` that some random variable `X` will take on a value less than or equal to the value on the `x` axis. In statistics, this can be used, for example, to find the probability that you will draw from a deck of cards the value between 2-8 (aces high). The CDF for such a scenaio is quite simple, since the probability of drawing any particular valued card is uniformly distributed at 1/13. Thus the CDF is:

$$
F_X(x) = \begin{cases}
\frac{x-1}{13} &:\ 2 \le x \le 14
\end{cases}
$$

#### CDF For Function Hotness :fire:

For the purposes of determining function hotness, we're looking at a CDF from a slightly different perspective. We're asking the question: "given a certain percentage $p$ (that being percentage of runtime), what is the edge weight threshold $F_h(p)$ such that the sum of all edge weights at or above $F_h(p)$ equals $p$ percentage of the total program runtime?" The answer $F_h(p)$ is the `hot-callsite-thres-from-CDF` value we saw Go print out, and $p$ is the `pgoinlinecdfthreshold` value we specified to the build process. We can mathematically describe our situation:


$$
W = \{w_0, w_1, ... w_n\}
$$

$$
w_i \gt w_{i+1}
$$

Where $W$ is the set of all edge weights in a program, ordered by descending value. We first need to find the value of positive integer $m$ such that the sum of the weights up to $m$ is approximately $p$. Why? Well because that's what the user is asking for when they specify `pgoinlinecdfthreshold`, they are asking the question "what nodes do I need to select (ordered by weight) such that their cumulative weight is approximately equal to `pgoinlinecdfthreshold`?"

This can be represented as 

$$
m \in \mathbb{W} 
$$

$$
F_h(W, p) = \frac{W_m}{\sum W} \quad \textrm{s.t.} \quad \frac{\sum_{i=0}^{min(m)} W_i}{\sum W} \gt p
$$

We select the smallest possible value $m$ that satisfies the inequality. I'm not sure if this is the most succinct way of describing this model but I'm not a mathemetician so you'll have to bear with me :smile: In English, $F_h(W, p)$ is the weight of the node at $W_m$ divided by the sum of all weights, such that the sum of the nodes from $0$ to $m$, divided by the sum of all the weights, is greater than $p$.

You can see the Go PGO logic implements this function [here](https://github.com/golang/go/blob/go1.21.0/src/cmd/compile/internal/inline/inl.go#L122-L155). More specifically, you can see that the returned `pgoinlinecdfthreshold` is indeed the percentage of the total edge weight that $W_m$ represents.

### Devirtualization

Another optimization technique that takes advantage of PGO is what's called devirutalization. In Go, interfaces provide a virtualized way of accessing an implementation that might not be known at compile time. These virutalized calls are inefficient because they involve jump tables that must be travesed in order to call the implementation's method during runtime. Interfaces are also problematic because [it defeats a lot of other analysis techniques like heap escapes](/blog/2023/07/15/analyzing-go-heap-escapes/#use-of-reference-types-on-interface-methods). 

We can still run profiles to see what concrete implementation in practice gets used the most. The compiler will then creare a small bit of if/else logic in the assembled code to do type assertions on the interface for the set of "hot" implementations found during profiling. If one of the type assertions succeeds, it will call that implementation's method directly.

We will not dive deeply into this specific optimization technique, but it's something to keep in mind and highlights various ways in which PGO can be leveraged. 

## Proving the CDF experimentally

Let's go back to our examples where we modified [`pgoinlinedthreshold`](#95). The calculated threshold value was `0.18328445747800587`, which according to the Go PGO code is the percentage of $W_m$ over the sum of all edge weights. The PGO logic does not have any debug statements that tells us what the total cumulative weight, so let's modify the Go source code with some additional print statements so we can confirm our calculations.

```diff title="src/cmd/compile/internal/inline/inl.go"
diff --git a/src/cmd/compile/internal/inline/inl.go b/src/cmd/compile/internal/inline/inl.go
index 4ae7fa95d2..56fdcfb099 100644
--- a/src/cmd/compile/internal/inline/inl.go
+++ b/src/cmd/compile/internal/inline/inl.go
@@ -89,6 +89,7 @@ func pgoInlinePrologue(p *pgo.Profile, decls []ir.Node) {
        inlineHotCallSiteThresholdPercent, hotCallsites = hotNodesFromCDF(p)
        if base.Debug.PGODebug > 0 {
                fmt.Printf("hot-callsite-thres-from-CDF=%v\n", inlineHotCallSiteThresholdPercent)
+               fmt.Printf("total-edge-weight=%v\n", p.TotalEdgeWeight)
        }
 
        if x := base.Debug.PGOInlineBudget; x != 0 {
@@ -145,6 +146,12 @@ func hotNodesFromCDF(p *pgo.Profile) (float64, []pgo.NodeMapKey) {
                w := p.NodeMap[n].EWeight
                cum += w
                if pgo.WeightInPercentage(cum, p.TotalEdgeWeight) > inlineCDFHotCallSiteThresholdPercent {
+                       fmt.Printf("node-that-exceeded-threshold-caller-name=%v\n", n.CallerName)
+                       fmt.Printf("node-that-exceeded-threshold-callee-name=%v\n", n.CalleeName)
+                       fmt.Printf("node-that-exceeded-threshold-edge-weight=%v\n", w)
+                       fmt.Printf("node-plus-one-caller-name=%v\n", nodes[i+1].CallerName)
+                       fmt.Printf("node-plus-one-callee-name=%v\n", nodes[i+1].CalleeName)
+                       fmt.Printf("node-plus-one-edge-weight=%v\n", p.NodeMap[nodes[i+1]].EWeight)
                        // nodes[:i+1] to include the very last node that makes it to go over the threshold.
                        // (Say, if the CDF threshold is 50% and one hot node takes 60% of weight, we want to
                        // include that node instead of excluding it.)
```

After compiling Go from source, we can run our build command again

```
$ ./goroot/bin/go build -pgo=auto -gcflags="-d=pgoinlinecdfthreshold=95,pgodebug=3" .
[...]
node-that-exceeded-threshold-caller-name=runtime.markrootBlock
node-that-exceeded-threshold-callee-name=runtime.scanblock
node-that-exceeded-threshold-edge-weight=25
node-plus-one-caller-name=syscall.Setenv
node-plus-one-callee-name=syscall.runtimeSetenv
node-plus-one-edge-weight=25
hot-callsite-thres-from-CDF=0.18328445747800587
total-edge-weight=13640
```

We can see that the node which caused the cumulative distribution to exceed the threshold was `runtime.scanblock`. Because it's part of the runtime, it was probably not included in our graph visualization. We can see that $\frac{25}{13640}*100\%=0.18328445747800587\%$ so it matches exactly the numbers that we're getting from `hot-callsite-thres-from-CDF`, which is no surprise. 


## Viewing the assembly

Let's have some fun an convince ourselves on what's really going on here. Sure these nice pretty graphs tell us that the PGO has inlined certain function calls, but why don't we take a look at the raw assembly code? First, let's look at the unoptimzed executable by building it with PGO turned off:

```
$ go build -pgo=off 
$ go tool objdump ./fermats-factorization |& less
```

By grepping for `main.go:34` we indeed find the location where `main.isSquare` is called on the function stack:

```
  main.go:34            0x6741a7                e814feffff              CALL main.isSquare(SB)                  
```

Let's build this again with PGO turned on, and for fun let's just rely on the default PGO values:

```
$ go build -pgo=auto -gcflags="-d=pgodebug=1" .  |& grep isSquare
hot-node enabled increased budget=2000 for func=main.isSquare
hot-budget check allows inlining for call main.NewExpensive (cost 130) at ./main.go:17:27 in function main.isSquare
hot-budget check allows inlining for call strconv.Itoa (cost 117) at ./main.go:18:43 in function main.isSquare
hot-budget check allows inlining for call os.Setenv (cost 90) at ./main.go:18:11 in function main.isSquare
hot-budget check allows inlining for call main.isSquare (cost 368) at ./main.go:34:12 in function main.findFactors
hot-budget check allows inlining for call main.isSquare (cost 368) at ./main.go:36:14 in function main.findFactors
```

Great! Even with the default parameters it still shows `main.isSquare` is allowed to be inlined. The graph visualization agrees:

![](/images/golang-profile-guided-optimizations/graphviz-threshold-defaults.svg)

What does the assembly say?

```
$ go tool objdump -s 'main.findFactors' ./fermats-factorization 
  main.go:30            0x6762e5                31f6                    XORL SI, SI                             
  main.go:33            0x6762e7                e91a010000              JMP 0x676406                            
  main.go:12            0x6762ec                4885c9                  TESTQ CX, CX                            
  main.go:12            0x6762ef                7c0a                    JL 0x6762fb                             
  main.go:12            0x6762f1                0f57c0                  XORPS X0, X0                            
  main.go:12            0x6762f4                f2480f2ac1              CVTSI2SDQ CX, X0                        
  main.go:12            0x6762f9                eb18                    JMP 0x676313                            
  main.go:12            0x6762fb                4889ce                  MOVQ CX, SI                             
  main.go:12            0x6762fe                83e101                  ANDL $0x1, CX  
```

We indeed see that the code in `isSquare` is being inlined directly in the assembly for `main.findFactors`.

## Conclusion

PGO is a really effective tool you can use to provide the compiler real-world examples of your code's CPU profile in a production system. The optimizations it provides are significant and are definitely worth the effort if reducing the latency in your applications is something you value. Let me know in the comments below what you think, and please do let me know if you see any errors that need correcting! 
