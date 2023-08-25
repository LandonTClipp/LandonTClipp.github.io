package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"strconv"
)

func isSquare(i uint64) (bool, uint64, uint64) {
	sqrt := math.Sqrt(float64(i))

	// We do something tricky here to exceed the Go inlining algorithm's
	// budget of 40 nodes. This call to NewExpensive has enough "code nodes"
	// to cause the compiler to consider isSquare too expensive to inline.
	expensive := NewExpensive()

	return sqrt == float64(uint64(sqrt)), uint64(sqrt), uint64(expensive)
}

func findFactors(n uint64) (uint64, uint64, uint64) {
	// Fermat’s Factorization
	for i := uint64(2); i < n; i++ {
		potentialSquare := n + (uint64(math.Pow(float64(i), 2.0)))

		// We dispense with typical go conventions like the function call and
		// boolean check being within the same if statement to make tracing
		// the assembly easier.
		issquare, sqrt, a := isSquare(uint64(potentialSquare))
		if issquare {
			return sqrt + i, sqrt - i, i
		}
		os.Setenv("A", strconv.Itoa(int(a)))
	}
	panic("no factors found")
}

func findFactorsALot(n uint64) (uint64, uint64, uint64) {
	var factor1, factor2, numIterations uint64
	// It turns out that Fermat's method is actually quite fast even for large
	// uint64 numbers, so arbitrarily do this a crap ton of times
	for i := 0; i < 99; i++ {
		factor1, factor2, numIterations = findFactors(n)
	}
	return factor1, factor2, numIterations
}

func main() {
	var nFlag = flag.Uint64("n", 8051, "integer to find prime for")
	flag.Parse()
	factor1, factor2, numIterations := findFactorsALot(*nFlag)
	fmt.Printf("Found factors with i=%d: %d = %d x %d\n", numIterations, *nFlag, factor1, factor2)

}
