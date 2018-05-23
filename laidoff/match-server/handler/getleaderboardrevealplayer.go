package handler

import (
	"net"
	"log"
	"github.com/lache/lo/laidoff/match-server/service"
	"github.com/lache/lo/laidoff/match-server/convert"
	"github.com/lache/lo/laidoff/shared-server"
)

func HandleGetLeaderboardRevealPlayer(buf []byte, conn net.Conn, serviceList *service.List) {
	log.Printf("GETLEADERBOARDREVEALPLAYER received")
	// Parse
	recvPacket, err := convert.ParseGetLeaderboardRevealPlayer(buf)
	if err != nil {
		log.Printf("HandleGetLeaderboardRevealPlayer fail: %v", err.Error())
		return
	}
	userId := convert.IdCuintToByteArray(recvPacket.Id)
	leaderboardRevealPlayerRequest := shared_server.LeaderboardRevealPlayerRequest{
		Id:    userId,
		Count: int(recvPacket.Count),
	}
	var leaderboardReply shared_server.LeaderboardReply
	err = serviceList.Rank.GetLeaderboardRevealPlayer(&leaderboardRevealPlayerRequest, &leaderboardReply)
	if err != nil {
		log.Printf("GetLeaderboardRevealPlayer fail: %v", err.Error())
		return
	} else {
		reply := convert.NewLwpLeaderboard(&leaderboardReply)
		replyBuf := convert.Packet2Buf(reply)
		conn.Write(replyBuf)
	}
}
