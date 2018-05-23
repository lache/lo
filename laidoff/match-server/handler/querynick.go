package handler

import (
	"log"
	"net"
	"github.com/lache/lo/laidoff/match-server/convert"
	"github.com/lache/lo/laidoff/db-server/user"
	"github.com/lache/lo/laidoff/db-server/dbservice"
	"github.com/lache/lo/laidoff/shared-server"
	"github.com/lache/lo/laidoff/rank-server/rankservice"
)

func HandleQueryNick(buf []byte, conn net.Conn, rankService rankservice.Rank, dbService dbservice.Db) {
	log.Printf("QUERYNICK received")
	recvPacket, err := convert.ParseQueryNick(buf)
	if err != nil {
		log.Printf("handleQueryNick fail: %v", err.Error())
		return
	}
	log.Printf("QUERYNICK User ID: %v", user.IdByteArrayToString(convert.IdCuintToByteArray(recvPacket.Id)))
	//userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	var userDb user.Db
	userId := convert.IdCuintToByteArray(recvPacket.Id)
	err = dbService.Get(&userId, &userDb)
	if err != nil {
		if err.Error() == "user db not exist" {
			// Client sends previously stored user ID, but server does not have that ID
			// Treat as a new user
			//userDb, _, err = user.CreateNewUser(convert.IdCuintToByteArray(recvPacket.Id), nickdb.PickRandomNick(ndb))
			err = dbService.Create(0, &userDb)
			if err != nil {
				log.Fatalf("load user db failed -> create new user failed")
			} else {
				newUserDataBuf := convert.Packet2Buf(convert.NewLwpNewUserData(userDb.Id, userDb.Nickname, userDb.Rating, -1))
				_, err = conn.Write(newUserDataBuf)
				if err != nil {
					log.Fatalf("NEWUSERDATA send failed: %v", err.Error())
				}
			}
		} else {
			log.Fatalf("load user db failed: %v", err)
		}
	} else {
		// Previously stored user db correctly loaded.
		// Reply to client
		log.Printf("User nick: %v", userDb.Nickname)
		var scoreRankItem shared_server.ScoreRankItem
		err = rankService.Get(&userDb.Id, &scoreRankItem)
		if err != nil {
			log.Printf("rank rpc get failed: %v", err.Error())
		}
		// Send a reply
		nickname := userDb.Nickname
		nickBuf := convert.Packet2Buf(convert.NewLwpNick(nickname, &scoreRankItem))
		_, err = conn.Write(nickBuf)
		if err != nil {
			log.Fatalf("LWPNICK send failed: %v", err.Error())
		}
	}
}
