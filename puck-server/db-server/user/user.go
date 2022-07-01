package user

import (
	"time"
	"fmt"
	"net"
	"io"
	"crypto/rand"
	"os"
	"errors"
	"log"
	"encoding/gob"
	"path/filepath"
)

type Id [16]byte
type LeaseId [16]byte

type BattleStat struct {
	// statistics which can be calculated only from REWARD service
	Victory               int // how many victories got so far
	Defeat                int // how many defeats got so far
	ConsecutiveVictory    int // how many consecutive victories got so far (reset to 0 when not victory)
	ConsecutiveDefeat     int // how many consecutive defeats got so far (reset to 0 when not defeat)
	MaxConsecutiveVictory int // highest record of ConsecutiveVictory (updated when it is less than ConsecutiveVictory)
	MaxConsecutiveDefeat  int // highest record of ConsecutiveDefeat (updated when it is less than ConsecutiveDefeat)
	Battle                int // how many battles finished so far
	// statistics which can be calculated from BATTLE service
	PerfectVictory  int // how many perfect victories (5:0) got so far
	PerfectDefeat   int // how many perfect defeats (0:5) got so far
	PuckWallHit     int // how many wall hits when puck is owned by player
	PuckTowerAttack int // how many tower hits to enemy when puck is owned by player
	PuckTowerDamage int // how many tower damages taken from enemy
	Dash            int // how many dashes
	TravelDistance  int // how many distance traveled so far (point 2 precision)
	MaxPuckSpeed    int // how fast owned puck moved (point 2 precision)
	BattleTimeSec   int // how many seconds elapsed of battling
	// statistics which can be calculated from MATCH service
	Search        int // how many searches started so far
	SearchWaitSec int // how many seconds elapsed while waiting
}

type Db struct {
	Id          Id         // unique id
	Created     time.Time  // creation date time
	Nickname    string     // nickname
	BattleStat  BattleStat // battle statistics
	LastLogin   time.Time  // last login date time
	WeeklyScore int        // weekly score
	Rating      int        // Elo rating
	Bot         bool       // bot
}

type LeaseDb struct {
	LeaseId LeaseId
	Db      Db
}

type Agent struct {
	Conn             net.Conn
	Db               Db
	CancelQueue      bool
	SupportedGameMap int
}

var persistentDbPath string

func SetPersistentDbPath(p string) {
	persistentDbPath = p
}

func GetPersistentDbPath() string {
	return persistentDbPath
}

func NewUuid() ([]byte, string, error) {
	uuid := make([]byte, 16)

	n, err := io.ReadFull(rand.Reader, uuid)
	if n != len(uuid) || err != nil {
		return nil, "", err
	}
	// variant bits; see section 4.1.1
	uuid[8] = uuid[8]&^0xc0 | 0x80
	// version 4 (pseudo-random); see section 4.1.3
	uuid[6] = uuid[6]&^0xf0 | 0x40
	return uuid, fmt.Sprintf("%08x-%08x-%08x-%08x", uuid[0:4], uuid[4:8], uuid[8:12], uuid[12:16]), nil
}

func IdByteArrayToString(id Id) string {
	return fmt.Sprintf("%08x-%08x-%08x-%08x", id[0:4], id[4:8], id[8:12], id[12:16])
}

func (db *Db) UuidStr() string {
	return IdByteArrayToString(db.Id)
}

func LoadUserDb(id Id) (*Db, error) {
	uuidStr := IdByteArrayToString(id)
	return LoadUserDbByUuidStr(uuidStr)
}

func LoadUserDbByUuidStr(uuidStr string) (*Db, error) {
	userDbFile, err := os.Open(filepath.Join(persistentDbPath, uuidStr))
	if err != nil {
		if os.IsNotExist(err) {
			// user db not exist
			return nil, errors.New("user db not exist")
		} else {
			log.Printf("disk open failed: %v", err.Error())
			return nil, err
		}
	} else {
		defer userDbFile.Close()
		decoder := gob.NewDecoder(userDbFile)
		userDb := &Db{}
		decoder.Decode(userDb)
		return userDb, nil
	}
}

func WriteUserDb(userDb *Db) error {
	userDbFile, err := os.Create(filepath.Join(persistentDbPath, IdByteArrayToString(userDb.Id)))
	if err != nil {
		log.Fatalf("User db file creation failed: %v", err.Error())
		return err
	}
	encoder := gob.NewEncoder(userDbFile)
	encoder.Encode(userDb)
	userDbFile.Close()
	log.Printf("DB written: %+v", userDb)
	return nil
}
