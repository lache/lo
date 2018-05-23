package rankservice

import (
	"time"
	"github.com/gasbank/laidoff/shared-server"
	"log"
	"math"
	"github.com/gasbank/laidoff/db-server/user"
)

// Rank is a struct containing a whole data a Rank service need to run.
type RankService struct {
	Rank              *RankData
	MatchPool         *RankData
	MatchPoolRequest  chan QueueScoreMatchRequestQueue
	matchPoolTimeBias time.Duration
}

// Set is a rpc call wrapper for SetWithNickname.
func (t *RankService) Set(args *shared_server.ScoreItem, reply *int) error {
	rank, _ := t.Rank.SetWithNickname(args.Id, args.Score, args.Nickname)
	*reply = rank
	return nil
}

// Get is a rpc call wrapper for Get.
func (t *RankService) Get(args *user.Id, reply *shared_server.ScoreRankItem) error {
	score, rank, _, err := t.Rank.Get(*args)
	if err != nil {
		log.Printf("Get failed: %v", err)
		reply.Id = *args
		reply.Score = 1500
		reply.Rank = -1
	} else {
		reply.Id = *args
		reply.Score = score
		reply.Rank = rank
	}
	return nil
}

// GetWithIndex is a rpc call wrapper for GetWithIndex.
func (t *RankService) GetWithIndex(args *user.Id, reply *shared_server.ScoreRankIndexItem) error {
	score, rank, _, index, err := t.Rank.GetWithIndex(*args)
	if err != nil {
		log.Printf("Get failed: %v", err)
		reply.Id = *args
		reply.Score = 1500
		reply.Rank = -1
		reply.Index = -1
	} else {
		reply.Id = *args
		reply.Score = score
		reply.Rank = rank
		reply.Index = index
	}
	return nil
}

// GetLeaderboard is a rpc call wrapper for getting a leaderboard data.
func (t *RankService) GetLeaderboard(args *shared_server.LeaderboardRequest, reply *shared_server.LeaderboardReply) error {
	scoreCount := len(t.Rank.ScoreArray)
	if scoreCount == 0 {
		log.Printf("Score empty")
		return nil
	}
	if scoreCount <= args.StartIndex || args.StartIndex < 0 {
		log.Printf("StartIndex out of bounds error")
		return nil
	}
	if args.Count <= 0 || args.Count > 100 {
		log.Printf("Count out of bounds error")
		return nil
	}

	firstId := t.Rank.IdArray[args.StartIndex]
	_, rank, tieCount, err := t.Rank.Get(firstId)
	if err != nil {
		log.Printf("Get error")
	} else {
		reply.FirstItemRank = rank
		reply.FirstItemTieCount = tieCount
		reply.RevealIndex = -1
		reply.CurrentPage = int(math.Ceil(float64(args.StartIndex+1) / float64(args.Count)))
		reply.TotalPage = int(math.Ceil(float64(scoreCount) / float64(args.Count)))
		items := make([]shared_server.LeaderboardItem, 0)
		c := args.Count
		if args.StartIndex+c > scoreCount {
			c = scoreCount - args.StartIndex
		}
		for i := 0; i < c; i++ {
			if t.Rank.IdArray[args.StartIndex+i] == args.Id {
				reply.RevealIndex = i
			}
			items = append(items, shared_server.LeaderboardItem{
				Nickname: t.Rank.NicknameArray[args.StartIndex+i],
				Score:    t.Rank.ScoreArray[args.StartIndex+i],
			})
		}
		reply.Items = items
	}
	return nil
}

// GetLeaderboardRevealPlayer is a rpc call wrapper for getting a leaderboard page revealing specific player.
func (t *RankService) GetLeaderboardRevealPlayer(args *shared_server.LeaderboardRevealPlayerRequest, reply *shared_server.LeaderboardReply) error {
	_, _, _, idArrayIndex, err := t.Rank.GetWithIndex(args.Id)
	var page, revealIndex int
	if err != nil {
		page = 0
		revealIndex = -1
	} else {
		page = int(idArrayIndex / args.Count)
		revealIndex = idArrayIndex - page*args.Count
	}
	req := shared_server.LeaderboardRequest{
		StartIndex: page * args.Count,
		Count:      args.Count,
	}
	err = t.GetLeaderboard(&req, reply)
	if err != nil {
		return err
	}
	// additionally, write RevealIndex field
	reply.RevealIndex = revealIndex
	return nil
}

type QueueScoreMatchRequestQueue struct {
	request   *QueueScoreMatchRequest
	replyChan chan QueueScoreMatchReply
}

func (t *RankService) QueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error {
	replyChan := make(chan QueueScoreMatchReply)
	t.MatchPoolRequest <- QueueScoreMatchRequestQueue{args, replyChan}
	r := <-replyChan
	if r.Err != nil {
		return r.Err
	}
	*reply = r
	return nil
}

func (t *RankService) commitQueueScoreMatch(args *QueueScoreMatchRequest, reply *QueueScoreMatchReply) error {
	if args.SetBias {
		t.matchPoolTimeBias = args.MatchPoolTimeBias
	} else if args.Flush {
		t.MatchPool.Flush()
		reply.RemoveNearestOverlapResult = &RemoveNearestOverlapResult{
			Matched:       false,
			NearestResult: nil,
		}
	} else {
		if args.Update {
			if _, exist := t.MatchPool.IdScoreMap[args.Id]; exist == false {
				reply.RemoveNearestOverlapResult = &RemoveNearestOverlapResult{
					AlreadyRemoved: true,
				}
				return nil
			}
		}
		if args.Delete {
			t.MatchPool.Remove(args.Id)
			reply.RemoveNearestOverlapResult = &RemoveNearestOverlapResult{
				AlreadyRemoved: true,
			}
			return nil
		}
		t.MatchPool.Set(args.Id, args.Score)
		now := time.Now().Add(t.matchPoolTimeBias)
		removeNearestOverlapResult, err := t.MatchPool.RemoveNearestOverlap(args.Id, &args.DistanceByElapsed, now)
		if err != nil {
			return err
		}
		reply.RemoveNearestOverlapResult = removeNearestOverlapResult
	}
	return nil
}

func NewLocalRankService() *RankService {
	rankService := NewLocalRankServiceWithTestSet(NewRank())
	go processQueueScoreMatch(*rankService)
	return rankService
}

func NewLocalRankServiceWithTestSet(rankData *RankData) *RankService {
	return &RankService{
		Rank:             rankData,
		MatchPool:        NewRank(),
		MatchPoolRequest: make(chan QueueScoreMatchRequestQueue),
	}
}

func processQueueScoreMatch(rankService RankService) {
	for {
		request := <-rankService.MatchPoolRequest
		var reply QueueScoreMatchReply
		err := rankService.commitQueueScoreMatch(request.request, &reply)
		if err != nil {
			reply.Err = err
		}
		request.replyChan <- reply
	}
}
