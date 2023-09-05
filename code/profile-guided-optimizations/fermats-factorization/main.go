package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"strconv"
)

func isSquare(i uint64) bool {
	sqrt := math.Sqrt(float64(i))

	// We do something tricky here to exceed the Go inlining algorithm's
	// budget of 40 nodes. This call to NewExpensive has enough "code nodes"
	// to cause the compiler to consider isSquare too expensive to inline.
	expensive := NewExpensive()
	os.Setenv("EXPENSIVE_VALUE", strconv.Itoa(int(expensive)))

	return sqrt == float64(uint64(sqrt))
}

func findFactors(n uint64) (uint64, uint64, uint64) {
	var a uint64
	var b2 uint64

	numIters := uint64(0)
	for a = uint64(math.Ceil(math.Sqrt(float64(n)))); ; a++ {
		numIters++
		b2 = a*a - n

		// We run isSquare many times in order to make it appear very hot
		for i := 0; i < 5; i++ {
			isSquare(b2)
		}
		if isSquare(b2) {
			break
		}

	}
	sqrtB := uint64(math.Sqrt(float64(b2)))
	return a - sqrtB, a + sqrtB, numIters
}

func main() {
	var nFlag = flag.Uint64("n", 8051, "integer to factor")
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
		if factor1*factor2 != *nFlag {
			panic(fmt.Sprintf("%d x %d != %d", factor1, factor2, *nFlag))
		}
		fmt.Printf("Found factors with i=%d: %d = %d x %d\n", numIterations, *nFlag, factor1, factor2)
	}

}
