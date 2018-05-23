package battle

import (
	"log"
	"bytes"
	"encoding/binary"
	"github.com/lache/lo/laidoff/db-server/user"
	"github.com/lache/lo/laidoff/match-server/config"
	"github.com/lache/lo/laidoff/match-server/convert"
	"net"
	"reflect"
	"errors"
	"unsafe"
	"github.com/lache/lo/laidoff/db-server/dbservice"
)

type Ok struct {
	RemoveCache    bool
	BattleId       int
	createBattleOk convert.CreateBattleOk
	c1             user.Agent
	c2             user.Agent
	RemoveUserId   user.Id
	GameMap        int
}

type Service struct {
	Conf config.ServerConfig
}

func (sc *Service) Connection() (net.Conn, error) {
	tcpAddr, err := net.ResolveTCPAddr(sc.Conf.BattleServiceConnType, sc.Conf.BattleServiceHost+":"+sc.Conf.BattleServicePort)
	if err != nil {
		log.Fatalf("Battle service ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Printf("Battle service DialTCP error! - %v", err.Error())
	}
	return conn, err
}

func WaitForReply(connToBattle net.Conn, replyPacketRef interface{}, expectedReplySize uintptr, expectedReplyType int) error {
	replyBuf := make([]byte, expectedReplySize)
	replyBufLen, err := connToBattle.Read(replyBuf)
	if err != nil {
		log.Printf("Recv %v reply failed - %v", reflect.TypeOf(replyPacketRef).String(), err.Error())
		return err
	}
	if replyBufLen != int(expectedReplySize) {
		log.Printf("Recv %v reply size error - %v expected, got %v", reflect.TypeOf(replyPacketRef).String(), expectedReplySize, replyBufLen)
		return errors.New("read size not match")
	}
	replyBufReader := bytes.NewReader(replyBuf)
	err = binary.Read(replyBufReader, binary.LittleEndian, replyPacketRef)
	if err != nil {
		log.Printf("binary.Read fail - %v", err)
		return err
	}
	packetSize := int(binary.LittleEndian.Uint16(replyBuf[0:2]))
	packetType := int(binary.LittleEndian.Uint16(replyBuf[2:4]))
	if packetSize == int(expectedReplySize) && packetType == expectedReplyType {
		return nil
	}
	return errors.New("parsed size or type error")
}

func createBattleInstance(battleService Service, c1 user.Agent, c2 user.Agent, battleOkQueue chan<- Ok, bot bool) {
	connToBattle, err := battleService.Connection()
	if err != nil {
		log.Printf("battleService error! %v", err.Error())
		SendRetryQueueLater(c1.Conn)
		SendRetryQueueLater(c2.Conn)
	} else {
		// Select game map (square or octagon)
		gameMap := convert.GetRandomGameMap(c1.SupportedGameMap, c1.Db.Rating, c2.SupportedGameMap, c2.Db.Rating)
		// Send create battle request
		createBattleBuf := convert.Packet2Buf(convert.NewCreateBattle(
			c1.Db.Id,
			c2.Db.Id,
			c1.Db.Nickname,
			c2.Db.Nickname,
			bot,
			gameMap,
		))
		_, err = connToBattle.Write(createBattleBuf)
		if err != nil {
			log.Fatalf("Send LSBPT_LWSPHEREBATTLEPACKETCREATEBATTLE failed")
		}
		// Wait for a reply
		createBattleOk, createBattleOkEnum := convert.NewLwpCreateBattleOk()
		err = WaitForReply(connToBattle, createBattleOk, unsafe.Sizeof(*createBattleOk), createBattleOkEnum)
		if err != nil {
			log.Printf("WaitForReply failed - %v", err.Error())
			SendRetryQueueLater(c1.Conn)
			SendRetryQueueLater(c2.Conn)
		} else {
			// No error! so far ... proceed battle
			if c2.Conn != nil {
				log.Printf("MATCH %v and %v matched successfully!", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
			} else {
				log.Printf("MATCH %v and *bot* matched successfully!", c1.Conn.RemoteAddr())
			}
			createBattleOkWrap := convert.CreateBattleOk{S: *createBattleOk}
			battleOkQueue <- Ok{
				false,
				int(createBattleOk.Battle_id),
				createBattleOkWrap,
				c1,
				c2,
				user.Id{},
				gameMap,
			}
		}
	}
}

func OkWorker(conf config.ServerConfig, battleOkQueue <-chan Ok, ongoingBattleMap map[user.Id]Ok) {
	for {
		battleOk := <-battleOkQueue
		if battleOk.RemoveCache == false {
			log.Printf("OkWorker: received battle ok. Sending MATCHED2 to both clients")
			ongoingBattleMap[battleOk.c1.Db.Id] = battleOk
			ongoingBattleMap[battleOk.c2.Db.Id] = battleOk
			WriteMatched2(conf, battleOk.c1.Conn, battleOk, battleOk.c1.Db.Id)
			WriteMatched2(conf, battleOk.c2.Conn, battleOk, battleOk.c2.Db.Id)
		} else {
			log.Printf("OkWorker: received battle ok [remove]. Removing cache entry for user %v", battleOk.RemoveUserId)
			delete(ongoingBattleMap, battleOk.RemoveUserId)
		}
	}
}

func WriteMatched2(conf config.ServerConfig, conn net.Conn, battleOk Ok, id user.Id) {
	if conn == nil {
		log.Printf("WriteMatched2 will be skipped since conn is nil. (maybe bot user?)")
		return
	}
	if battleOk.c1.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, uint(battleOk.createBattleOk.S.C1_token), 1, battleOk.c1.Db.Rating, battleOk.c2.Db.Rating, battleOk.c2.Db.Nickname, battleOk.GameMap))
	} else if battleOk.c2.Db.Id == id {
		conn.Write(createMatched2Buf(conf, battleOk.createBattleOk, uint(battleOk.createBattleOk.S.C2_token), 2, battleOk.c2.Db.Rating, battleOk.c1.Db.Rating, battleOk.c1.Db.Nickname, battleOk.GameMap))
	}
}

func createMatched2Buf(conf config.ServerConfig, createBattleOk convert.CreateBattleOk, token uint, playerNo, playerScore, targetScore int, targetNickname string, gameMap int) []byte {
	publicAddr, err := net.ResolveTCPAddr(conf.BattlePublicServiceConnType, conf.BattlePublicServiceHost+":"+conf.BattlePublicServicePort)
	if err != nil {
		log.Panicf("BattlePublicService conf parse error: %v", err.Error())
	}
	publicAddrIpv4 := publicAddr.IP.To4()

	return convert.Packet2Buf(convert.NewLwpMatched2(
		publicAddr.Port,
		publicAddrIpv4,
		int(createBattleOk.S.Battle_id),
		token,
		playerNo,
		playerScore,
		targetScore,
		targetNickname,
		gameMap,
	))
}

func SendRetryQueue(conn net.Conn) {
	retryQueueBuf := convert.Packet2Buf(convert.NewLwpRetryQueue())
	_, retrySendErr := conn.Write(retryQueueBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry match packet to client", conn.RemoteAddr())
	}
}

func SendRetryQueue2(conn net.Conn, queueType int) {
	retryQueueBuf := convert.Packet2Buf(convert.NewLwpRetryQueue2(queueType))
	_, retrySendErr := conn.Write(retryQueueBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry match packet to client", conn.RemoteAddr())
	}
}

func SendRetryQueueLater(conn net.Conn) {
	if conn == nil {
		return
	}
	retryQueueLaterBuf := convert.Packet2Buf(convert.NewLwpRetryQueueLater())
	_, retrySendErr := conn.Write(retryQueueLaterBuf)
	if retrySendErr != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), retrySendErr.Error())
	} else {
		log.Printf("%v: Send retry later match packet to client", conn.RemoteAddr())
	}
}

func Create1vs1Match(c1 user.Agent, c2 user.Agent, battleService Service, battleOkQueue chan<- Ok, queueType int) {
	log.Printf("%v and %v matched! (maybe)", c1.Conn.RemoteAddr(), c2.Conn.RemoteAddr())
	maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
	n1, err1 := c1.Conn.Write(maybeMatchedBuf)
	n2, err2 := c2.Conn.Write(maybeMatchedBuf)
	if n1 == 4 && n2 == 4 && err1 == nil && err2 == nil {
		go createBattleInstance(battleService, c1, c2, battleOkQueue, false)
	} else {
		switch queueType {
		case convert.LWPUCKGAMEQUEUETYPENEARESTSCORE:
			// Match cannot be proceeded
			checkMatchError2(err1, c1.Conn, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE)
			checkMatchError2(err2, c2.Conn, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE)
		case convert.LWPUCKGAMEQUEUETYPEFIFO:
			fallthrough
		default:
			// Match cannot be proceeded
			checkMatchError(err1, c1.Conn)
			checkMatchError(err2, c2.Conn)
		}
	}
}

func CreateBotMatch(c1 user.Agent, battleService Service, battleOkQueue chan<- Ok, queueType int, db dbservice.Db) {
	log.Printf("%v and *bot* matched! (maybe)", c1.Conn.RemoteAddr())
	maybeMatchedBuf := convert.Packet2Buf(convert.NewLwpMaybeMatched())
	n1, err1 := c1.Conn.Write(maybeMatchedBuf)
	var botUserDb user.Db
	err := db.Create(1, &botUserDb)
	if err != nil {
		log.Printf("Create error: %v", err.Error())
		return
	}
	c2Bot := user.Agent{
		Conn:             nil,
		Db:               botUserDb,
		CancelQueue:      false,
		SupportedGameMap: convert.LPGMOCTAGON,
	}
	if n1 == 4 && err1 == nil {
		go createBattleInstance(battleService, c1, c2Bot, battleOkQueue, true)
	} else {
		switch queueType {
		case convert.LWPUCKGAMEQUEUETYPENEARESTSCORE:
			// Match cannot be proceeded
			checkMatchError2(err1, c1.Conn, convert.LWPUCKGAMEQUEUETYPENEARESTSCORE)
		case convert.LWPUCKGAMEQUEUETYPEFIFO:
			fallthrough
		default:
			// Match cannot be proceeded
			checkMatchError(err1, c1.Conn)
		}
	}
}

func checkMatchError(err error, conn net.Conn) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		SendRetryQueue(conn)
	}
}

func checkMatchError2(err error, conn net.Conn, queueType int) {
	if err != nil {
		log.Printf("%v: %v error!", conn.RemoteAddr(), err.Error())
	} else {
		SendRetryQueue2(conn, queueType)
	}
}
