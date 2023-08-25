package main

import (
	"fmt"
	"log"
	"os"
	"runtime/pprof"
)

func profile(cpuprofile string) (close func()) {
	fmt.Println("starting CPU profile")
	f, err := os.Create(cpuprofile)
	if err != nil {
		log.Fatal("could not create CPU profile: ", err)
	}

	if err := pprof.StartCPUProfile(f); err != nil {
		log.Fatal("could not start CPU profile: ", err)
	}

	close = func() {
		pprof.StopCPUProfile()
		f.Close()
	}
	return close
}
