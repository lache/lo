package handler

import (
	"log"
	"net"
	"puck-server/db-server/dbservice"
	"puck-server/db-server/user"
	"puck-server/match-server/convert"
)

func HandleNewUser(conn net.Conn, dbService dbservice.Db) {
	log.Printf("NEWUSER received")
	var userDb user.Db
	err := dbService.Create(0, &userDb)
	if err != nil {
		log.Printf("DB service Create failed: %v", err.Error())
	} else {
		// reply to client
		newUserDataBuf := convert.Packet2Buf(convert.NewLwpNewUserData(userDb.Id, userDb.Nickname, userDb.Rating, -1))
		_, err = conn.Write(newUserDataBuf)
		if err != nil {
			log.Fatalf("NEWUSERDATA send failed: %v", err.Error())
		}
	}
}
