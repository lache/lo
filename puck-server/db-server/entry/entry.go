package entry

import (
	"encoding/gob"
	"errors"
	"io/ioutil"
	"log"
	"math/rand"
	"net"
	"net/rpc"
	"os"
	"path/filepath"
	"puck-server/db-server/dbadmin"
	"puck-server/db-server/dbservice"
	"puck-server/db-server/nickdb"
	"puck-server/db-server/user"
	"strings"
	"time"
)

type LeaseData struct {
	LeaseId  user.LeaseId
	LeasedAt time.Time
}

type LeaseReply struct {
	LeaseDb *user.LeaseDb
	err     error
}

type LeaseWriteRequest struct {
	Id             *user.Id
	LeaseReplyChan chan LeaseReply
	LeaseDb        *user.LeaseDb
	WriteReplyChan chan WriteReply
}

type WriteReply struct {
	Reply int
	err   error
}

type DbService struct {
	nickDb                 nickdb.NickDb
	botNickDb              nickdb.NickDb
	leaseMap               map[user.Id]LeaseData
	leaseWriteRequestQueue chan LeaseWriteRequest
}

func (t *DbService) Create(args int, reply *user.Db) error {
	// create a new test user and write db to filesystem
	uuid, uuidStr, err := user.NewUuid()
	if err != nil {
		log.Printf("[Service] new uuid failed: %v", err.Error())
		return err
	}
	var newNick string
	if args == 0 {
		newNick = nickdb.PickRandomNick(&t.nickDb)
	} else {
		newNick = nickdb.PickRandomNick(&t.botNickDb)
	}
	// Write to disk
	var id user.Id
	copy(id[:], uuid)
	userDb, _, err := createNewUser(id, newNick, args != 0, user.GetPersistentDbPath())
	if err != nil {
		log.Printf("[Service] createNewUser failed: %v", err.Error())
		return err
	}
	*reply = *userDb
	log.Printf("[Service] Create ok (user ID %v, %v)", uuidStr, uuid)
	return nil
}

func (t *DbService) Get(args *user.Id, reply *user.Db) error {
	db, err := user.LoadUserDb(*args)
	if err != nil {
		return err
	}
	log.Printf("[Service] Get ok (user ID %v)", args)
	*reply = *db
	return nil
}

func (t *DbService) Lease(args *user.Id, reply *user.LeaseDb) error {
	replyChan := make(chan LeaseReply)
	t.leaseWriteRequestQueue <- LeaseWriteRequest{args, replyChan, nil, nil}
	r := <-replyChan
	if r.err != nil {
		return r.err
	} else {
		*reply = *r.LeaseDb
	}
	return nil
}

func CommitLease(t *DbService, leaseRequest *LeaseWriteRequest) {
	args := *leaseRequest.Id
	leaseData, exist := t.leaseMap[args]
	if exist == false {
		// You can lease
		db, err := user.LoadUserDb(args)
		if err != nil {
			leaseRequest.LeaseReplyChan <- LeaseReply{nil, err}
			return
		}
		leaseIdSlice, _, err := user.NewUuid()
		if err != nil {
			leaseRequest.LeaseReplyChan <- LeaseReply{nil, err}
			return
		}
		var leaseId user.LeaseId
		copy(leaseId[:], leaseIdSlice)
		t.leaseMap[args] = LeaseData{leaseId, time.Now()}
		leaseRequest.LeaseReplyChan <- LeaseReply{&user.LeaseDb{leaseId, *db}, nil}
		log.Printf("[Service] Lease ok (user ID %v)", args)
	} else {
		leaseDuration := time.Now().Sub(leaseData.LeasedAt)
		if leaseDuration > 1*time.Second {
			log.Printf("Lease data already exists, but it is too old. (%v) Force remove lease data...", leaseDuration)
			delete(t.leaseMap, args)
			CommitLease(t, leaseRequest)
			return
		}
		leaseRequest.LeaseReplyChan <- LeaseReply{nil, errors.New("already leased")}
		return
	}
}

func (t *DbService) Write(args *user.LeaseDb, reply *int) error {
	replyChan := make(chan WriteReply)
	t.leaseWriteRequestQueue <- LeaseWriteRequest{nil, nil, args, replyChan}
	r := <-replyChan
	if r.err != nil {
		return r.err
	} else {
		*reply = r.Reply
	}
	return nil
}

func (t *DbService) GetAllUserRatings(args *dbservice.GetAllUserRatingsRequest, reply *dbservice.GetAllUserRatingsReply) error {
	files, err := ioutil.ReadDir(user.GetPersistentDbPath())
	if err != nil {
		return err
	} else {
		*reply = dbservice.GetAllUserRatingsReply{}
		for _, f := range files {
			userDb, err := user.LoadUserDbByUuidStr(f.Name())
			if err != nil {
				return err
			} else {
				(*reply).Id = append((*reply).Id, userDb.Id)
				(*reply).Nickname = append((*reply).Nickname, userDb.Nickname)
				(*reply).Rating = append((*reply).Rating, userDb.Rating)
			}
		}
	}
	return nil
}

func CommitWrite(t *DbService, writeRequest *LeaseWriteRequest) {
	leaseData, exist := t.leaseMap[writeRequest.LeaseDb.Db.Id]
	if exist {
		if leaseData.LeaseId == writeRequest.LeaseDb.LeaseId {
			err := user.WriteUserDb(&writeRequest.LeaseDb.Db)
			if err != nil {
				log.Printf("writeUserDb failed with error: %v", err.Error())
				writeRequest.WriteReplyChan <- WriteReply{0, err}
				return
			}
			delete(t.leaseMap, writeRequest.LeaseDb.Db.Id)
			log.Printf("[Service] Write user ok (nickname %v, user ID %v)",
				writeRequest.LeaseDb.Db.Nickname, writeRequest.LeaseDb.Db.Id)
			writeRequest.WriteReplyChan <- WriteReply{1, nil}
		} else {
			writeRequest.WriteReplyChan <- WriteReply{0, errors.New("lease not match")}
			return
		}
	} else {
		writeRequest.WriteReplyChan <- WriteReply{0, errors.New("lease not found")}
		return
	}
}

func ProcessLeaseWriteRequest(t *DbService) {
	for {
		request := <-t.leaseWriteRequestQueue
		if request.LeaseReplyChan != nil {
			CommitLease(t, &request)
		} else if request.WriteReplyChan != nil {
			CommitWrite(t, &request)
		}
	}
}

const (
	ServiceName = "db"
	ServiceAddr = ":20181"
)

func Entry() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Printf("Greetings from %v service", ServiceName)
	// Seed random generator
	rand.Seed(time.Now().UnixNano())
	// Check for Docker environment
	dir, err := os.Getwd()
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("Working directory: %v", dir)
	if strings.HasPrefix(dir, "/app/") {
		log.Printf("Working directory starts with '/app/'. Assuming Docker container environment...")
		user.SetPersistentDbPath("/data/db")
	} else {
		user.SetPersistentDbPath("db")
	}
	log.Printf("Persistent Db Path: %v", user.GetPersistentDbPath())
	// Create db directory to save user database
	os.MkdirAll(user.GetPersistentDbPath(), os.ModePerm)
	//createTestUserDb()
	server := rpc.NewServer()
	dbService := new(DbService)
	dbService.nickDb = nickdb.LoadNickDbByFilename("adj.txt", "noun.txt")
	dbService.botNickDb = nickdb.LoadNickDbByFilename("adj.txt", "bot-noun.txt")
	dbService.leaseMap = make(map[user.Id]LeaseData)
	dbService.leaseWriteRequestQueue = make(chan LeaseWriteRequest)
	go ProcessLeaseWriteRequest(dbService)
	server.RegisterName("DbService", dbService)
	log.Printf("Listening %v for db service...", ServiceAddr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", ServiceAddr)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	go dbadmin.StartService()
	// This statement links rpc server to the socket, and allows rpc server to accept
	// rpc request coming from that socket.
	server.Accept(l)
}

func createNewUser(uuid user.Id, nickname string, bot bool, persistentDbPath string) (*user.Db, *os.File, error) {
	userDb := &user.Db{
		Id:       uuid,
		Created:  time.Now(),
		Nickname: nickname,
		Rating:   1500,
		Bot:      bot,
	}
	uuidStr := user.IdByteArrayToString(uuid)
	userDbFile, err := os.Create(filepath.Join(persistentDbPath, uuidStr))
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return nil, nil, err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	return userDb, userDbFile, nil
}
