package handler

import (
	"log"
	"net"
	"puck-server/match-server/convert"
	"puck-server/match-server/service"
	"puck-server/shared-server"
)

func HandleGetLeaderboard(buf []byte, conn net.Conn, serviceList *service.List) {
	log.Printf("GETLEADERBOARD received")
	// Parse
	recvPacket, err := convert.ParseGetLeaderboard(buf)
	if err != nil {
		log.Printf("HandleGetLeaderboard fail: %v", err.Error())
	}
	leaderboardRequest := shared_server.LeaderboardRequest{
		Id:         convert.IdCuintToByteArray(recvPacket.Id),
		StartIndex: int(recvPacket.Start_index),
		Count:      int(recvPacket.Count),
	}
	var leaderboardReply shared_server.LeaderboardReply
	err = serviceList.Rank.GetLeaderboard(&leaderboardRequest, &leaderboardReply)
	if err != nil {
		log.Printf("rank rpc error: %v", err.Error())
	} else {
		reply := convert.NewLwpLeaderboard(&leaderboardReply)
		replyBuf := convert.Packet2Buf(reply)
		conn.Write(replyBuf)
	}
}
