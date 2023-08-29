package main

import (
	"os"
)

func yIfLongest(x, y *string) *string {
	if len(*x) > len(*y) {
		return y
	}
	xReference := x
	return xReference
}

func main() {
	x := "ab"
	y := "abcde"

	result := yIfLongest(&x, &y)
	os.Setenv("Y", *result)
}
