package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"strconv"
)

func isSquare(i uint64) (bool, uint64) {
	sqrt := math.Sqrt(float64(i))

	// We do something tricky here to exceed the Go inlining algorithm's
	// budget of 40 nodes. This call to NewExpensive has enough "code nodes"
	// to cause the compiler to consider isSquare too expensive to inline.
	expensive := NewExpensive()
	os.Setenv("EXPENSIVE_VALUE", strconv.Itoa(int(expensive)))

	return sqrt == float64(uint64(sqrt)), uint64(sqrt)
}

func findFactors(n uint64) (uint64, uint64, uint64) {
	// Fermat’s Factorization
	for i := uint64(2); i < n; i++ {
		potentialSquare := n + (uint64(math.Pow(float64(i), 2.0)))

		var issquare bool
		var sqrt uint64
		// We call isSquare redundantly many times in order to artificially inflate its
		// runtime weight to induce it to be inlined later on.
		for j := 0; j < 20; j++ {
			issquare, sqrt = isSquare(potentialSquare)
		}

		if issquare {
			return sqrt + i, sqrt - i, i
		}
	}
	panic("no factors found")
}

func main() {
	var nFlag = flag.Uint64("n", 8051, "integer to find prime for")
	var cpuprofile = flag.String("cpuprofile", "default.pgo", "write cpu profile to `file`")
	var httpProfile = flag.Bool("httpprof", false, "")
	var infinite = flag.Int("infinite", 0, "")

	flag.Parse()

	// Start the CPU profiling routines using runtime/pprof
	close := runtimeProf(*cpuprofile)
	defer close()

	if *httpProfile {
		httpProf()
	}

	if *infinite != 0 {
		for {
			findFactors(*nFlag)
		}
	} else {
		factor1, factor2, numIterations := findFactors(*nFlag)
		fmt.Printf("Found factors with i=%d: %d = %d x %d\n", numIterations, *nFlag, factor1, factor2)
	}

}
