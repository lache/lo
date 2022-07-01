package handler

import (
	"log"
	"net"
	"puck-server/db-server/dbservice"
	"puck-server/db-server/user"
	"puck-server/match-server/convert"
	"regexp"
)

func HandleSetNickname(buf []byte, conn net.Conn, dbService dbservice.Db) {
	log.Printf("SETNICKNAME received")
	// Parse
	recvPacket, err := convert.ParseSetNickname(buf)
	if err != nil {
		log.Printf("HandleSetNickname fail: %v", err.Error())
		return
	}
	//userDb, err := user.LoadUserDb(convert.IdCuintToByteArray(recvPacket.Id))
	var userLeaseDb user.LeaseDb
	userId := convert.IdCuintToByteArray(recvPacket.Id)
	err = dbService.Lease(&userId, &userLeaseDb)
	if err != nil {
		log.Printf("user db load failed: %v", err.Error())
	} else {
		var newNickname string
		convert.CCharArrayToGoString(&recvPacket.Nickname, &newNickname)
		newNicknameByteLen := len([]byte(newNickname))
		if newNicknameByteLen >= convert.LWNICKNAMEMAXLEN-4 {
			queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket, convert.LWSETNICKNAMERESULTTOOLONG))
			conn.Write(queueOkBuf)
			log.Printf("Nickname '%v' (byte len %v) too long.", newNickname, newNicknameByteLen)
			return
		} else if newNicknameByteLen < 4 {
			queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket, convert.LWSETNICKNAMERESULTTOOSHORT))
			conn.Write(queueOkBuf)
			log.Printf("Nickname '%v' (byte len %v) too short.", newNickname, newNicknameByteLen)
			return
		} else {
			nicknameRegexp := regexp.MustCompile("^[A-Za-z0-9가-힣]+$")
			if nicknameRegexp.MatchString(newNickname) == false {
				queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket, convert.LWSETNICKNAMERESULTTOONOTALLOWED))
				conn.Write(queueOkBuf)
				log.Printf("Nickname '%v' (byte len %v) contains not allowed characters.", newNickname, newNicknameByteLen)
				return
			}
		}
		// good to go
		oldNickname := userLeaseDb.Db.Nickname
		userLeaseDb.Db.Nickname = newNickname
		//user.WriteUserDb(userDb)
		var writeReply int
		err = dbService.Write(&userLeaseDb, &writeReply)
		if err != nil {
			queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket, convert.LWSETNICKNAMERESULTINTERNALERROR))
			conn.Write(queueOkBuf)
			log.Printf("DB service Write failed: %v", err)
		} else {
			queueOkBuf := convert.Packet2Buf(convert.NewLwpSetNicknameResult(recvPacket, convert.LWSETNICKNAMERESULTOK))
			conn.Write(queueOkBuf)
			log.Printf("Nickname '%v' changed to '%v'", oldNickname, userLeaseDb.Db.Nickname)
		}
	}
}
