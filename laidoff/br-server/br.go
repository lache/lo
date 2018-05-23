package main

import (
	dbentry "github.com/gasbank/laidoff/db-server/entry"
	matchentry "github.com/gasbank/laidoff/match-server/entry"
	pushentry "github.com/gasbank/laidoff/push-server/entry"
	rankentry "github.com/gasbank/laidoff/rank-server/entry"
	rewardentry "github.com/gasbank/laidoff/reward-server/entry"
	"os"
	"log"
)

func main() {
	argsLen := len(os.Args)
	if argsLen < 2 {
		log.Printf("Should specify entry points from one of the following: db, match, push, rank, reward")
		return
	}
	if os.Args[1] == "db" {
		dbentry.Entry()
	}
	if os.Args[1] == "match" {
		matchentry.Entry()
	}
	if os.Args[1] == "push" {
		pushentry.Entry()
	}
	if os.Args[1] == "rank" {
		rankentry.Entry()
	}
	if os.Args[1] == "reward" {
		rewardentry.Entry()
	}
}
