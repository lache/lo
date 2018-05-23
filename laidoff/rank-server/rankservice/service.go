package rankservice

import (
	"github.com/lache/lo/laidoff/match-server/rpchelper"
	"github.com/lache/lo/laidoff/shared-server"
	"github.com/lache/lo/laidoff/db-server/user"
	"time"
	"log"
)

type Context struct {
	rpcContext *rpchelper.Context
}

type QueueScoreMatchRequest struct {
	Flush             bool              // flush all score match queue
	SetBias           bool              // set match wait time bias (for debugging)
	MatchPoolTimeBias time.Duration     // amount of match wait time bias (for debugging)
	Id                user.Id           // queue user ID
	Score             int               // queue score
	DistanceByElapsed DistanceByElapsed // allowed match score range data
	Update            bool              // true if this is a update request
	Delete            bool              // true if this is a delete request
}

type QueueScoreMatchReply struct {
	RemoveNearestOverlapResult *RemoveNearestOverlapResult
	Err                        error
}

type DistanceByElapsed struct {
	Elapsed  []time.Duration
	Distance []int
}

func (t *DistanceByElapsed) FindDistance(elapsed time.Duration) int {
	for i, e := range t.Elapsed {
		if e <= elapsed {
			return t.Distance[i]
		}
	}
	log.Printf("FindDistance data error")
	return 100
}

type NearestResult struct {
	Id                  user.Id
	Score               int
	IdArrayIndex        int
	NearestId           user.Id
	NearestScore        int
	NearestIdArrayIndex int
}

type RemoveNearestOverlapResult struct {
	Matched        bool
	AlreadyRemoved bool
	NearestResult  *NearestResult
}

// Set a new score entry
func (c *Context) Set(args *shared_server.ScoreItem, reply *int) error {
	return c.rpcContext.Call("Set", args, reply)
}

// Get score and Rank
func (c *Context) Get(args *user.Id, reply *shared_server.ScoreRankItem) error {
	return c.rpcContext.Call("Get", args, reply)
}

// List a leaderboard
func (c *Context) GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error {
	return c.rpcContext.Call("GetLeaderboard", args, reply)
}

// List a leaderboard page revealing specific user
func (c *Context) GetLeaderboardRevealPlayer(args *shared_server.LeaderboardRevealPlayerRequest, reply *shared_server.LeaderboardReply) error {
	return c.rpcContext.Call("GetLeaderboardRevealPlayer", args, reply)
}

func (c *Context) QueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error {
	return c.rpcContext.Call("QueueScoreMatch", args, reply)
}

// Get score, Rank and index
func (c *Context) GetWithIndex(args *user.Id, reply *shared_server.ScoreRankIndexItem) error {
	return c.rpcContext.Call("GetWithIndex", args, reply)
}

func New(address string) Rank {
	c := new(Context)
	c.rpcContext = rpchelper.New("Rank", address)
	return c
}

type Rank interface {
	// Set a new score entry
	Set(args *shared_server.ScoreItem, reply *int) error
	// Get score and Rank
	Get(args *user.Id, reply *shared_server.ScoreRankItem) error
	// List a leaderboard
	GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error
	// List a leaderboard page revealing specific user
	GetLeaderboardRevealPlayer(args *shared_server.LeaderboardRevealPlayerRequest, reply *shared_server.LeaderboardReply) error
	// Queue a user for score-based match
	QueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error
	// Get score, Rank and index
	GetWithIndex(args *user.Id, reply *shared_server.ScoreRankIndexItem) error
}
