package main

import (
	"log"
	"net/http"
	_ "net/http/pprof"
)

func httpProf() {
	go func() {
		log.Println(http.ListenAndServe("localhost:6060", nil))
	}()
}
