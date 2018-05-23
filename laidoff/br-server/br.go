package main

import (
	dbentry "github.com/lache/lo/laidoff/db-server/entry"
	matchentry "github.com/lache/lo/laidoff/match-server/entry"
	pushentry "github.com/lache/lo/laidoff/push-server/entry"
	rankentry "github.com/lache/lo/laidoff/rank-server/entry"
	rewardentry "github.com/lache/lo/laidoff/reward-server/entry"
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
