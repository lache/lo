package entry

import (
	"encoding/binary"
	"encoding/json"
	"io"
	"log"
	"math/rand"
	"net"
	"os"
	"sync"
	"time"

	"puck-server/db-server/user"
	"puck-server/match-server/battle"
	"puck-server/match-server/config"
	"puck-server/match-server/convert"
	"puck-server/match-server/handler"
	"puck-server/match-server/service"
	"puck-server/rank-server/rankservice"
	"puck-server/shared-server"
)

const (
	ServiceName = "match"
)

func CopyFile(fromName, toName string) {
	from, err := os.Open(fromName)
	if err != nil {
		log.Fatal(err)
	}
	defer from.Close()

	to, err := os.OpenFile(toName, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		log.Fatal(err)
	}
	defer to.Close()

	_, err = io.Copy(to, from)
	if err != nil {
		log.Fatal(err)
	}
}

func Entry() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Printf("Greetings from %v service", ServiceName)
	// Test Db serviceLW_PUCK_GAME_QUEUE_TYPE_NEAREST_SCORE
	if len(os.Args) > 1 && os.Args[1] == "test" {
		service.CreateNewUserDb()
	}
	// Seed a new random number
	rand.Seed(time.Now().Unix())
	// Service List
	serviceList := service.NewServiceList()
	// Load conf.json
	confFile, err := os.Open("conf.json")
	if err != nil {
		log.Printf("trying to create conf.json from conf.json.template...")
		CopyFile("conf.json.template", "conf.json")
		confFile, err = os.Open("conf.json")
		if err != nil {
			log.Fatalf("conf.json open error:%v", err.Error())
		}
	}
	confFileDecoder := json.NewDecoder(confFile)
	conf := config.ServerConfig{}
	err = confFileDecoder.Decode(&conf)
	if err != nil {
		log.Fatalf("conf.json parse error:%v", err.Error())
	}
	// override values from conf.json if env is set
	if os.Getenv("BATTLE_HOST") != "" {
		log.Printf("BattlePublicServiceHost changed from %v to %v by environment variable BATTLE_HOST", conf.BattlePublicServiceHost, os.Getenv("BATTLE_HOST"))
		conf.BattlePublicServiceHost = os.Getenv("BATTLE_HOST")
	}
	if os.Getenv("BATTLE_PORT") != "" {
		log.Printf("BattlePublicServicePort changed from %v to %v by environment variable BATTLE_PORT", conf.BattlePublicServicePort, os.Getenv("BATTLE_PORT"))
		conf.BattlePublicServicePort = os.Getenv("BATTLE_PORT")
	}
	if os.Getenv("LOBBY_PORT") != "" {
		log.Printf("ConnPort changed from %v to %v by environment variable LOBBY_PORT", conf.ConnPort, os.Getenv("LOBBY_PORT"))
		conf.ConnPort = os.Getenv("LOBBY_PORT")
	}
	// Test RPC
	//testRpc(serviceList)
	// Create 1 vs. 1 match waiting queue
	matchQueue := make(chan user.Agent)
	// Create battle ok queue
	battleOkQueue := make(chan battle.Ok)
	// Ongoing battle map
	ongoingBattleMap := make(map[user.Id]battle.Ok)
	// Nearest match map (QUEUE3)
	nearestMatchMap := make(map[user.Id]user.Agent)
	nearestMatchMapLock := sync.RWMutex{}
	// Battle service
	battleService := battle.Service{Conf: conf}
	// Start match worker goroutine
	go matchWorker(battleService, matchQueue, battleOkQueue, serviceList)
	// Start battle ok worker goroutine
	go battle.OkWorker(conf, battleOkQueue, ongoingBattleMap)
	// Open TCP service port and listen for game clients
	l, err := net.Listen(conf.ConnType, conf.ConnHost+":"+conf.ConnPort)
	if err != nil {
		log.Fatalln("Error listening:", err.Error())
	}
	defer l.Close()
	log.Printf("Listening %v for match service...", conf.ConnHost+":"+conf.ConnPort)
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Println("Error accepting: ", err.Error())
		} else {
			req := &HandleRequestRequest{
				Conf:                conf,
				Conn:                conn,
				MatchQueue:          matchQueue,
				Battle:              battleService,
				ServiceList:         serviceList,
				OngoingBattleMap:    ongoingBattleMap,
				BattleOkQueue:       battleOkQueue,
				NearestMatchMap:     nearestMatchMap,
				NearestMatchMapLock: nearestMatchMapLock,
			}
			go handleRequest(req)
		}
	}
}

func testRankRpc(rankService rankservice.Rank, id byte, score int, nickname string) {
	scoreItem := shared_server.ScoreItem{Id: user.Id{id}, Score: score, Nickname: nickname}
	var reply int
	err := rankService.Set(&scoreItem, &reply)
	if err != nil {
		log.Printf("rank rpc error: %v", err.Error())
	} else {
		log.Println(reply)
	}
}

//noinspection GoUnusedFunction
func testRpc(serviceList *service.List) {
	pushToken := shared_server.PushToken{
		Domain:    500,
		PushToken: "test-push-token",
		UserId:    user.Id{1, 2, 3, 4},
	}
	var registerReply int
	log.Println(serviceList.Push.RegisterPushToken(&pushToken, &registerReply))
	testRankRpc(serviceList.Rank, 1, 100, "TestUser1")
	testRankRpc(serviceList.Rank, 2, 200, "TestUser2")
	testRankRpc(serviceList.Rank, 3, 300, "TestUser3")
	testRankRpc(serviceList.Rank, 4, 50, "TestUser4")
	testRankRpc(serviceList.Rank, 5, 20, "TestUser5")
	testRankRpc(serviceList.Rank, 6, 20, "TestUser6")
	testRankRpc(serviceList.Rank, 7, 20, "TestUser7")
	testRankRpc(serviceList.Rank, 8, 10, "TestUser8")
	leaderboardRequest := shared_server.LeaderboardRequest{
		StartIndex: 0,
		Count:      20,
	}
	var leaderboardReply shared_server.LeaderboardReply
	err := serviceList.Rank.GetLeaderboard(&leaderboardRequest, &leaderboardReply)
	if err != nil {
		log.Printf("rank rpc get leaderboard returned error: %v", err.Error())
	} else {
		log.Println(leaderboardReply)
	}
	userId := user.Id{5}
	var scoreRankItem shared_server.ScoreRankItem
	err = serviceList.Rank.Get(&userId, &scoreRankItem)
	if err != nil {
		log.Printf("rank rpc get returned error: %v", err.Error())
	} else {
		log.Println(scoreRankItem)
	}
}

func matchWorker(battleService battle.Service, matchQueue <-chan user.Agent, battleOkQueue chan<- battle.Ok, serviceList *service.List) {
	for {
		log.Printf("Match queue empty")
		c1 := <-matchQueue
		log.Printf("Match queue size 1")
		broadcastPush := shared_server.BroadcastPush{
			Title: "RUMBLE",
			Body:  c1.Db.Nickname + " provokes you!",
		}
		var broadcastReply int
		log.Printf("Broadcast result(return error): %v", serviceList.Push.Broadcast(&broadcastPush, &broadcastReply))
		c2 := <-matchQueue
		log.Printf("Match queue size 2")
		if c1.Conn == c2.Conn {
			if c2.CancelQueue {
				// Send queue cancel success reply
				cancelQueueOkBuf := convert.Packet2Buf(convert.NewLwpCancelQueueOk())
				c2.Conn.Write(cancelQueueOkBuf)
				log.Printf("Nickname '%v' queue cancelled", c2.Db.Nickname)
			} else {
				log.Printf("The same connection sending QUEUE2 twice. Flushing match requests and replying with RETRYQUEUE to the later connection...")
				battle.SendRetryQueue(c2.Conn)
			}
		} else if c1.Db.Id == c2.Db.Id {
			log.Printf("The same user ID sending QUEUE2 twice. Flushing match requests and replying with RETRYQUEUE to the later connection...")
			battle.SendRetryQueue(c2.Conn)
		} else {
			battle.Create1vs1Match(c1, c2, battleService, battleOkQueue, convert.LWPUCKGAMEQUEUETYPEFIFO)
		}
	}
}

type HandleRequestRequest struct {
	Conf                config.ServerConfig
	Conn                net.Conn
	MatchQueue          chan<- user.Agent
	Battle              battle.Service
	ServiceList         *service.List
	OngoingBattleMap    map[user.Id]battle.Ok
	BattleOkQueue       chan<- battle.Ok
	NearestMatchMap     map[user.Id]user.Agent
	NearestMatchMapLock sync.RWMutex
}

func handleRequest(req *HandleRequestRequest) {
	log.Printf("Accepting from %v", req.Conn.RemoteAddr())
	previousBuf := make([]byte, 0)
	for {
		//conn.SetReadDeadline(time.Now().Add(10000 * time.Millisecond))
		currentBuf := make([]byte, 1024)
		currentLen, err := req.Conn.Read(currentBuf)
		if currentLen == 0 {
			log.Printf("%v: currentLen is zero.", req.Conn.RemoteAddr())
			break
		}
		if err != nil {
			log.Printf("%v: Error reading: %v", req.Conn.RemoteAddr(), err.Error())
		}
		log.Printf("%v: Packet received (currentLen=%v)", req.Conn.RemoteAddr(), currentLen)
		currentBuf = currentBuf[:currentLen]

		buf := currentBuf
		if len(previousBuf) > 0 {
			buf = append(previousBuf, currentBuf...)
		}
		log.Printf("  bufLen %v", len(buf))
		for len(buf) >= 4 {
			packetSize := binary.LittleEndian.Uint16(buf)
			log.Printf("%v: Packet size specified from first 16-bit is %v", req.Conn.RemoteAddr(), int(packetSize))
			if len(buf) < int(packetSize) {
				log.Printf("%v: len(buf)[%v] is less than int(packetSize)[%v]!", req.Conn.RemoteAddr(), len(buf), int(packetSize))
				break
			}
			if int(packetSize) < 4 {
				log.Printf("%v: Packet size[%v] is too small!", req.Conn.RemoteAddr(), int(packetSize))
				break
			}
			msgBuf := buf[:packetSize]
			packetType := binary.LittleEndian.Uint16(msgBuf[2:])
			log.Printf("  readLen %v", len(msgBuf))
			log.Printf("  Size %v", packetSize)
			log.Printf("  Type %v", packetType)

			switch packetType {
			case convert.LPGPLWPQUEUE2:
				handler.HandleQueue2(req.Conf, req.MatchQueue, msgBuf, req.Conn, req.OngoingBattleMap, req.Battle, req.BattleOkQueue, req.ServiceList.Db)
			case convert.LPGPLWPQUEUE3:
				req := &handler.HandleQueue3Request{
					Conf:                req.Conf,
					NearestMatchMap:     req.NearestMatchMap,
					NearestMatchMapLock: req.NearestMatchMapLock,
					MatchQueue:          req.MatchQueue,
					Buf:                 msgBuf,
					Conn:                req.Conn,
					OngoingBattleMap:    req.OngoingBattleMap,
					BattleService:       req.Battle,
					BattleOkQueue:       req.BattleOkQueue,
					Db:                  req.ServiceList.Db,
					Rank:                req.ServiceList.Rank,
				}
				handler.HandleQueue3(req)
			case convert.LPGPLWPCANCELQUEUE:
				req := &handler.HandleCancelQueueRequest{
					MatchQueue:          req.MatchQueue,
					Buf:                 msgBuf,
					Conn:                req.Conn,
					OngoingBattleMap:    req.OngoingBattleMap,
					Db:                  req.ServiceList.Db,
					NearestMatchMap:     req.NearestMatchMap,
					NearestMatchMapLock: req.NearestMatchMapLock,
					Rank:                req.ServiceList.Rank,
				}
				handler.HandleCancelQueue(req)
			case convert.LPGPLWPSUDDENDEATH:
				handler.HandleSuddenDeath(req.Conf, msgBuf) // relay 'msgBuf' to battle service
			case convert.LPGPLWPNEWUSER:
				handler.HandleNewUser(req.Conn, req.ServiceList.Db)
			case convert.LPGPLWPQUERYNICK:
				handler.HandleQueryNick(msgBuf, req.Conn, req.ServiceList.Rank, req.ServiceList.Db)
			case convert.LPGPLWPPUSHTOKEN:
				handler.HandlePushToken(msgBuf, req.Conn, req.ServiceList)
			case convert.LPGPLWPGETLEADERBOARD:
				handler.HandleGetLeaderboard(msgBuf, req.Conn, req.ServiceList)
			case convert.LPGPLWPGETLEADERBOARDREVEALPLAYER:
				handler.HandleGetLeaderboardRevealPlayer(msgBuf, req.Conn, req.ServiceList)
			case convert.LPGPLWPSETNICKNAME:
				handler.HandleSetNickname(msgBuf, req.Conn, req.ServiceList.Db)
			default:
				// Unknown packet should be ignored
				break
			}
			buf = buf[packetSize:]
		}
		previousBuf = buf
	}
	req.Conn.Close()
	log.Printf("Conn closed %v", req.Conn.RemoteAddr())
}
