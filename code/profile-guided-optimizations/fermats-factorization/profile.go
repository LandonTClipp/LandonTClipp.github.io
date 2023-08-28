package main

import (
	"fmt"
	"log"
	"net/http"
	httpPprof "net/http/pprof"
	"os"
	"runtime/pprof"
)

func runtimeProf(cpuprofile string) (close func()) {
	if cpuprofile == "" {
		return func() {}
	}
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

func httpProf() {
	go func() {
		log.Println(http.ListenAndServe("localhost:6060", httpPprof.Handler("")))
	}()
}
