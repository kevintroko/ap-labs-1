// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 227.

// Netcat is a simple read/write client for TCP servers.
package main

import (
	"flag"
	"io"
	"log"
	"net"
	"os"
)

var (
	user    string
	address string
)

func init() {
	flag.StringVar(&user, "user", "user", "flag for setting user")
	flag.StringVar(&address, "server", "localhost:8080", "flag for setting address")
}

//!+
func main() {
	flag.Parse()

	conn, err := net.Dial("tcp", address)
	if err != nil {
		log.Fatal(err)
	}
	conn.Write([]byte(string(user)))

	done := make(chan struct{})
	go func() {
		io.Copy(os.Stdout, conn) // NOTE: ignoring errors
		log.Println("done")
		done <- struct{}{} // signal the main goroutine
	}()
	mustCopy(conn, os.Stdin)
	conn.Close()
	<-done // wait for background goroutine to finish
}

//!-

func mustCopy(dst io.Writer, src io.Reader) {
	if _, err := io.Copy(dst, src); err != nil {
		log.Fatal(err)
	}
}
