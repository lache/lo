package handler

import (
	"net"
	"log"
	"github.com/lache/lo/laidoff/db-server/user"
	"github.com/lache/lo/laidoff/match-server/convert"
	"github.com/lache/lo/laidoff/match-server/battle"
	"github.com/lache/lo/laidoff/db-server/dbservice"
	"sync"
	"github.com/lache/lo/laidoff/rank-server/rankservice"
)

type HandleCancelQueueRequest struct {
	MatchQueue          chan<- user.Agent
	Buf                 []byte
	Conn                net.Conn
	OngoingBattleMap    map[user.Id]battle.Ok
	Db                  dbservice.Db
	NearestMatchMap     map[user.Id]user.Agent
	NearestMatchMapLock sync.RWMutex
	Rank                rankservice.Rank
}

func HandleCancelQueue(req *HandleCancelQueueRequest) {
	log.Printf("CANCELQUEUE received")
	recvPacket, err := convert.ParseCancelQueue(req.Buf)
	if err != nil {
		log.Printf("HandleCancelQueue fail: %v", err.Error())
		return
	}
	var userDb user.Db
	userId := convert.IdCuintToByteArray(recvPacket.Id)
	err = req.Db.Get(&userId, &userDb)
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		// Check ongoing battle
		_, battleExists := req.OngoingBattleMap[userDb.Id]
		if battleExists {
			log.Printf("Nickname '%v' has the ongoing battle session. CANCELQUEUE?!", userDb.Nickname)
		}
		req.NearestMatchMapLock.RLock()
		_, existOnNearestMatchMap := req.NearestMatchMap[userId]
		req.NearestMatchMapLock.RUnlock()
		if existOnNearestMatchMap {
			deleteFromQueueReq := &DeleteFromQueueRequest{
				Id:                  userId,
				Rank:                req.Rank,
				NearestMatchMap:     req.NearestMatchMap,
				NearestMatchMapLock: req.NearestMatchMapLock,
			}
			DeleteFromQueue(deleteFromQueueReq)
		} else {
			// Queue 'cancel queue' request
			req.MatchQueue <- user.Agent{req.Conn, userDb, true, 0}
		}
	}
}
