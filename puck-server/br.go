package main

import (
	"log"
	"os"
	dbEntry "puck-server/db-server/entry"
	matchEntry "puck-server/match-server/entry"
	pushEntry "puck-server/push-server/entry"
	rankEntry "puck-server/rank-server/entry"
	rewardEntry "puck-server/reward-server/entry"
)

func main() {
	argsLen := len(os.Args)
	if argsLen < 2 {
		log.Printf("Should specify entry points from one of the following: db, match, push, rank, reward")
		return
	}
	if os.Args[1] == "db" {
		dbEntry.Entry()
	}
	if os.Args[1] == "match" {
		matchEntry.Entry()
	}
	if os.Args[1] == "push" {
		pushEntry.Entry()
	}
	if os.Args[1] == "rank" {
		rankEntry.Entry()
	}
	if os.Args[1] == "reward" {
		rewardEntry.Entry()
	}
}
