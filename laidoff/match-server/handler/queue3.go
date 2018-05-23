package handler

import (
	"net"
	"log"
	"github.com/gasbank/laidoff/db-server/user"
	"github.com/gasbank/laidoff/match-server/convert"
	"github.com/gasbank/laidoff/match-server/battle"
	"github.com/gasbank/laidoff/match-server/config"
	"github.com/gasbank/laidoff/db-server/dbservice"
	"github.com/gasbank/laidoff/rank-server/rankservice"
	"time"
	"sync"
	"math/rand"
)

type HandleQueue3Request struct {
	Conf                config.ServerConfig
	NearestMatchMap     map[user.Id]user.Agent
	NearestMatchMapLock sync.RWMutex
	MatchQueue          chan<- user.Agent
	Buf                 []byte
	Conn                net.Conn
	OngoingBattleMap    map[user.Id]battle.Ok
	BattleService       battle.Service
	BattleOkQueue       chan<- battle.Ok
	Db                  dbservice.Db
	Rank                rankservice.Rank
}

func HandleQueue3(req *HandleQueue3Request) {
	log.Printf("QUEUE3 received")
	recvPacket, err := convert.ParseQueue3(req.Buf)
	if err != nil {
		log.Printf("ParseQueue3 error: %v", err.Error())
		return
	}
	switch recvPacket.QueueType {
	case convert.LWPUCKGAMEQUEUETYPENEARESTSCORE:
		userId := convert.IdCuintToByteArray(recvPacket.Id)
		handleNearestScoreQueue3(userId, convert.LPGMSQUARE, req)
	case convert.LWPUCKGAMEQUEUETYPENEARESTSCOREWITHOCTAGONSUPPORT:
		userId := convert.IdCuintToByteArray(recvPacket.Id)
		handleNearestScoreQueue3(userId, convert.LPGMOCTAGON, req)
	case convert.LWPUCKGAMEQUEUETYPEFIFO:
		fallthrough
	default:
		HandleQueue2(req.Conf, req.MatchQueue, req.Buf, req.Conn, req.OngoingBattleMap, req.BattleService, req.BattleOkQueue, req.Db)
	}
}

func handleNearestScoreQueue3(userId user.Id, supportedGameMap int, req *HandleQueue3Request) {
	userDb, resumed, err := CheckOngoingBattle(userId, req.Db, req.OngoingBattleMap, req.BattleService, req.BattleOkQueue, req.Conn, req.Conf)
	if err != nil {
		log.Printf("CheckOngoingBattle error: %v", err.Error())
	} else if resumed == false {
		req.NearestMatchMapLock.RLock()
		_, exist := req.NearestMatchMap[userDb.Id]
		req.NearestMatchMapLock.RUnlock()
		if exist {
			// ignore duplicated queueing req
			log.Printf("Duplicated queue req detected from user %v", userDb.Id)
		} else {
			// moment of truth
			// match with bot if user defeated at previous battle
			if userDb.BattleStat.ConsecutiveDefeat > 0 {
				log.Printf("Here comes a defeat type bot...!")
				agent1 := user.Agent{Conn: req.Conn, Db: userDb, SupportedGameMap: supportedGameMap}
				battle.CreateBotMatch(agent1, req.BattleService, req.BattleOkQueue, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE, req.Db)
			} else {
				// normal pvp
				// queue user-requested score match entry
				go queueToScoreMatch(userDb, supportedGameMap, req)
			}
		}
	}
}

func queueToScoreMatch(userDb user.Db, supportedGameMap int, req *HandleQueue3Request) {
	timeDilationFactor := 0.5 + 0.5*rand.Float64()
	distanceByElapsed := rankservice.DistanceByElapsed{
		Elapsed: []time.Duration{
			//35 * time.Second,
			//29 * time.Second,
			//19 * time.Second,
			time.Duration(timeDilationFactor*9000) * time.Millisecond,
			time.Duration(timeDilationFactor*3000) * time.Millisecond,
			time.Duration(timeDilationFactor*0) * time.Millisecond,
		},
		Distance: []int{// aware that this is a 'half-'distance...
			//130, // after 35 sec
			//100, // ~35 sec
			//60,  // ~29 sec
			75, // ~19 sec
			45, // ~9 sec
			15,  // ~3 sec
		},
	}
	queueScoreMatchReq := rankservice.QueueScoreMatchRequest{
		Flush:             false,
		SetBias:           false,
		MatchPoolTimeBias: 0,
		Id:                userDb.Id,
		Score:             userDb.Rating,
		DistanceByElapsed: distanceByElapsed,
		Update:            false, // user requested queueing
	}
	lastElapsedMargin := time.Duration(timeDilationFactor*3000) * time.Millisecond
	reqInterval := 500 * time.Millisecond
	reqTotalTime := distanceByElapsed.Elapsed[0] + lastElapsedMargin
	reqMaxCount := int(reqTotalTime / reqInterval)
	log.Printf("ScoreMatch queue %v (score:%v) [reqTotalTime=%v,reqMaxCount=%v,reqInterval=%v]", userDb.Nickname, userDb.Rating, reqTotalTime, reqMaxCount, reqInterval)
	for reqCount := 0; reqCount < reqMaxCount; reqCount++ {
		var queueScoreMatchReply rankservice.QueueScoreMatchReply
		err := req.Rank.QueueScoreMatch(&queueScoreMatchReq, &queueScoreMatchReply)
		if err != nil {
			log.Printf("QueueScoreMatch error: %v", err.Error())
		} else {
			if req.Conn != nil && queueScoreMatchReq.Update == false {
				// Cache this match request at first time
				req.NearestMatchMapLock.Lock()
				req.NearestMatchMap[userDb.Id] = user.Agent{Conn: req.Conn, Db: userDb, SupportedGameMap: supportedGameMap}
				req.NearestMatchMapLock.Unlock()
				// Send reply to client at first time
				queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
				req.Conn.Write(queueOkBuf)
				log.Printf("Nickname '%v' newly queued on nearest match map", userDb.Nickname)
			}
			if matchResult := queueScoreMatchReply.RemoveNearestOverlapResult; matchResult != nil {
				if matchResult.Matched {
					req.NearestMatchMapLock.RLock()
					agent1, exist1 := req.NearestMatchMap[matchResult.NearestResult.Id]
					agent2, exist2 := req.NearestMatchMap[matchResult.NearestResult.NearestId]
					req.NearestMatchMapLock.RUnlock()
					log.Printf("ScoreMatch matched 1: %v (score:%v) exist %v", agent1.Db.Nickname, agent1.Db.Rating, exist1)
					log.Printf("ScoreMatch matched 2: %v (score:%v) exist %v", agent2.Db.Nickname, agent2.Db.Rating, exist2)
					if exist1 && exist2 {
						req.NearestMatchMapLock.Lock()
						delete(req.NearestMatchMap, matchResult.NearestResult.Id)
						delete(req.NearestMatchMap, matchResult.NearestResult.NearestId)
						req.NearestMatchMapLock.Unlock()
						battle.Create1vs1Match(agent1, agent2, req.BattleService, req.BattleOkQueue, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE)
						return
					} else {
						if exist1 == false {
							log.Printf("agent1 not exist...?")
						}
						if exist2 == false {
							log.Printf("agent2 not exist...?")
						}
						// go directly to RETRYQUEUE2
						break
					}
				} else if matchResult.AlreadyRemoved {
					log.Printf("%v already removed from score match pool", userDb.Nickname)
					return
				}
			}
		}
		// wait some time
		time.Sleep(reqInterval)
		// change to 'update' type queue after first try
		queueScoreMatchReq.Update = true
	}
	log.Printf("reqMaxCount %v (%v) exceeded for score match request user %v", reqMaxCount, reqInterval*time.Duration(reqMaxCount), userDb.Nickname)
	req.NearestMatchMapLock.Lock()
	delete(req.NearestMatchMap, userDb.Id)
	req.NearestMatchMapLock.Unlock()

	log.Printf("Here comes a long-wait type bot...!")
	agent1 := user.Agent{Conn: req.Conn, Db: userDb, SupportedGameMap: supportedGameMap}
	battle.CreateBotMatch(agent1, req.BattleService, req.BattleOkQueue, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE, req.Db)
	// or wait again
	//battle.SendRetryQueue2(req.Conn, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE)
}

type DeleteFromQueueRequest struct {
	Id                  user.Id
	Rank                rankservice.Rank
	NearestMatchMap     map[user.Id]user.Agent
	NearestMatchMapLock sync.RWMutex
}

func DeleteFromQueue(req *DeleteFromQueueRequest) {
	req.NearestMatchMapLock.RLock()
	_, existOnNearestMatchMap := req.NearestMatchMap[req.Id]
	req.NearestMatchMapLock.RUnlock()
	if existOnNearestMatchMap {
		queueScoreMatchReq := rankservice.QueueScoreMatchRequest{
			Id:     req.Id,
			Delete: true,
		}
		var queueScoreMatchReply rankservice.QueueScoreMatchReply
		err := req.Rank.QueueScoreMatch(&queueScoreMatchReq, &queueScoreMatchReply)
		if err != nil {
			log.Printf("QueueScoreMatch error: %v", err.Error())
		} else if queueScoreMatchReply.RemoveNearestOverlapResult.AlreadyRemoved {
			req.NearestMatchMapLock.Lock()
			delete(req.NearestMatchMap, req.Id)
			req.NearestMatchMapLock.Unlock()
			log.Printf("Deleted from score match queue (user ID %v)", req.Id)
		} else {
			log.Printf("Logic error - delete request failed (user ID %v)", req.Id)
		}
	} else {
		log.Printf("User ID not exist on score match match map (user ID %v)", req.Id)
	}
}
