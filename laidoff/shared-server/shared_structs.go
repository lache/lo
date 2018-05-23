package shared_server

import "github.com/gasbank/laidoff/db-server/user"

type Args struct {
	A, B int
}

type Quotient struct {
	Quo, Rem int
}

type PushToken struct {
	Domain    int
	PushToken string
	UserId    user.Id
}

type BroadcastPush struct {
	Title string
	Body  string
}

type ScoreItem struct {
	Id       user.Id
	Score    int
	Nickname string
}

type ScoreRankItem struct {
	Id    user.Id
	Score int
	Rank  int
}

type ScoreRankIndexItem struct {
	Id    user.Id
	Score int
	Rank  int
	Index int
}

type LeaderboardRequest struct {
	Id         user.Id
	StartIndex int
	Count      int
}

type LeaderboardRevealPlayerRequest struct {
	Id    user.Id
	Count int
}

type LeaderboardItem struct {
	Nickname string
	Score    int
}

type LeaderboardReply struct {
	FirstItemRank     int
	FirstItemTieCount int
	RevealIndex       int
	Items             []LeaderboardItem
	CurrentPage       int // one-based, 0 on empty leaderboard
	TotalPage         int // one-based, 0 on empty leaderboard
}
