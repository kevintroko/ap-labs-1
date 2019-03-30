// Copyright © 2016 Alan A. A. Donovan & Brian W. Kernighan.
// License: https://creativecommons.org/licenses/by-nc-sa/4.0/

// See page 254.
//!+

// Chat is a server that lets clients chat with each other.
package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"net"
	"strings"
	"time"
)

//!+broadcaster
type client chan<- string // an outgoing message channel

// defines a user
type user struct {
	Username string
	IP       string
}

var (
	entering    = make(chan client)
	leaving     = make(chan client)
	messages    = make(chan string) // all incoming client messages
	addrToUsers = make(map[string]*user)
	addrToConn  = make(map[string]net.Conn)
	host        string
	port        int
)

func init() {
	flag.StringVar(&host, "host", "localhost", "flag for setting host")
	flag.IntVar(&port, "port", 8080, "flag for setting port")
}

func broadcaster() {
	clients := make(map[client]bool) // all connected clients
	for {
		select {
		case msg := <-messages:
			// Broadcast incoming message to all
			// clients' outgoing message channels.
			for cli := range clients {
				cli <- msg
			}

		case cli := <-entering:
			clients[cli] = true

		case cli := <-leaving:
			delete(clients, cli)
			close(cli)
		}
	}
}

//!-broadcaster

//!+handleConn
func handleConn(conn net.Conn) {
	ch := make(chan string) // outgoing client messages
	go clientWriter(conn, ch)

	who := conn.RemoteAddr().String()

	ch <- fmt.Sprintf("irc-server > Welcome to the IRC Server\nirc-server > Your user [%s] is successfully logged", addrToUsers[who].Username)
	fmt.Printf("irc-server > New connected user [%s]\n", addrToUsers[who].Username)
	messages <- fmt.Sprintf("user %s", addrToUsers[who].Username) + " has arrived"
	entering <- ch

	input := bufio.NewScanner(conn)
	for input.Scan() {
		messages <- addrToUsers[who].Username + ": " + input.Text()
	}
	// NOTE: ignoring potential errors from input.Err()

	leaving <- ch
	fmt.Printf("[%s] left\n", addrToUsers[who].Username)
	messages <- fmt.Sprintf("%s", addrToUsers[who].Username) + " has left"
	delete(addrToUsers, who)
	delete(addrToConn, who)

	conn.Close()
}

func clientWriter(conn net.Conn, ch <-chan string) {
	for msg := range ch {

		fmt.Fprintln(conn, msg) // NOTE: ignoring network errors
		if strings.Contains(msg, "/users") {
			resp := "irc-server > "
			for _, user := range addrToUsers {
				resp += user.Username
			}
			conn.Write([]byte(resp)) //ignoring error

			continue
		}
		if strings.Contains(msg, "/time") {
			resp := "irc-server > " + time.Now().String()
			conn.Write([]byte(resp)) //ignoring error

			continue
		}
		if strings.Contains(msg, "/user ") {
			resp := "irc-server > "
			userMsg := strings.Split(msg, " ")
			for _, user := range addrToUsers {
				if user.Username == userMsg[1] {
					resp += fmt.Sprintf("username: %s, IP: %s", user.Username, user.IP)
					break
				}
			}
			conn.Write([]byte(resp))

			continue
		}
		if strings.Contains(msg, "/msg ") {
			resp := "irc-server > "
			var destConn net.Conn
			userMsg := strings.Split(msg, " ")
			for _, user := range addrToUsers {
				if user.Username == userMsg[1] {
					destConn = addrToConn[user.IP]
					for i := 2; i < len(userMsg); i++ {
						resp += userMsg[i] + " "
					}
					break
				}
			}
			strings.Trim(resp, " ")
			destConn.Write([]byte(resp))

			continue
		}
	}
}

//!-handleConn

//!+main
func main() {
	flag.Parse()
	address := fmt.Sprintf("%s:%d", host, port)
	listener, err := net.Listen("tcp", address)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("irc-server > Simple IRC Server started at %s\n", address)
	fmt.Println("irc-server > Ready for receiving new clients")

	go broadcaster()
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Print(err)
			continue
		}
		// body := bufio.NewScanner(conn).Text()
		input := bufio.NewScanner(conn).Text()
		body := string(input)

		fmt.Println(string(body))
		addr := conn.RemoteAddr().String()
		addrToUsers[addr] = &user{
			Username: body,
			IP:       addr,
		}
		addrToConn[addr] = conn

		go handleConn(conn)
	}

}

//!-main
