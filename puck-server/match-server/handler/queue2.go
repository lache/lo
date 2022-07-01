package handler

import (
	"log"
	"net"
	"puck-server/db-server/dbservice"
	"puck-server/db-server/user"
	"puck-server/match-server/battle"
	"puck-server/match-server/config"
	"puck-server/match-server/convert"
	"unsafe"
)

func HandleQueue2(conf config.ServerConfig, matchQueue chan<- user.Agent, buf []byte, conn net.Conn, ongoingBattleMap map[user.Id]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok, dbService dbservice.Db) {
	log.Printf("QUEUE2 received")
	recvPacket, err := convert.ParseQueue2(buf)
	if err != nil {
		log.Printf("ParseQueue2 error: %v", err.Error())
		return
	}
	userId := convert.IdCuintToByteArray(recvPacket.Id)
	userDb, resumed, err := CheckOngoingBattle(userId, dbService, ongoingBattleMap, battleService, battleOkQueue, conn, conf)
	if err != nil {
		log.Printf("CheckOngoingBattle error: %v", err.Error())
	} else if resumed == false {
		// Queue connection
		matchQueue <- user.Agent{conn, userDb, false, convert.LPGMSQUARE}
		// Send reply
		queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
		conn.Write(queueOkBuf)
		log.Printf("Nickname '%v' queued", userDb.Nickname)
	}
}

func CheckOngoingBattle(userId user.Id, dbService dbservice.Db, ongoingBattleMap map[user.Id]battle.Ok, battleService battle.Service, battleOkQueue chan<- battle.Ok, conn net.Conn, conf config.ServerConfig) (userDb user.Db, resumed bool, err error) {
	resumed = false
	err = nil
	err = dbService.Get(&userId, &userDb)
	if err != nil {
		// error set
	} else {
		// Check ongoing battle
		battleOk, battleExists := ongoingBattleMap[userDb.Id]
		if battleExists {
			// There is the ongoing battle for this user
			log.Printf("Nickname '%v' has the ongoing battle session. Check this battle still valid...", userDb.Nickname)
			connToBattle, err := battleService.Connection()
			if err != nil {
				// However, we could not check whether this session is valid or not
				// since battle service is not available
				log.Printf("Connection to battle service failed - %v", err.Error())
				log.Printf("Assume ongoing battle session is invalid")
				// Remove from ongoing battle map(cache)
				battleOkQueue <- battle.Ok{
					RemoveCache:  true,
					RemoveUserId: userDb.Id,
				}
				// And then fallback to queuing this user
			} else {
				// Successfully connected to battle service.
				checkBattleValidBuf := convert.Packet2Buf(convert.NewCheckBattleValid(battleOk.BattleId))
				connToBattle.Write(checkBattleValidBuf)
				battleValid, battleValidEnum := convert.NewLwpBattleValid()
				err = battle.WaitForReply(connToBattle, battleValid, unsafe.Sizeof(*battleValid), battleValidEnum)
				if err != nil {
					log.Printf("WaitForReply failed - %v", err.Error())
				} else {
					if battleValid.Valid == 1 {
						// There is an ongoing battle session here.
						// [1] QUEUEOK
						queueOkBuf := convert.Packet2Buf(convert.NewLwpQueueOk())
						conn.Write(queueOkBuf)
						// [2] MAYBEMATCHED
						maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
						conn.Write(maybeMatchedBuf)
						// [3] MATCHED2
						battle.WriteMatched2(conf, conn, battleOk, userDb.Id)
						// Should exit this function here
						resumed = true
					} else {
						// Battle is not valid. Remove from cache
						battleOkQueue <- battle.Ok{
							RemoveCache:  true,
							RemoveUserId: userDb.Id,
						}
						// And then fallback to queuing this user
					}
				}
			}
		}
	}
	return userDb, resumed, err
}
