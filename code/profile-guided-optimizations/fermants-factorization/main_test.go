package main

import (
	"fmt"
	"testing"
)

func BenchmarkFindFactors(b *testing.B) {
	var fact1, fact2, numIterations uint64
	for i := 0; i < b.N; i++ {
		fact1, fact2, numIterations = findFactorsALot(10976191241513578168)
	}
	fmt.Printf("numIterations=%d fact1=%d fact2=%d\n", numIterations, fact1, fact2)
}
