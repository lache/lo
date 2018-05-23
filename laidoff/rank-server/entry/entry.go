package entry

import (
	"log"
	"net/rpc"
	"net"
	"github.com/lache/lo/laidoff/rank-server/rankservice"
	"os"
	"github.com/lache/lo/laidoff/db-server/dbservice"
)

func registerRankService(rpcServer *rpc.Server, rankInterface rankservice.Rank) {
	rpcServer.RegisterName("Rank", rankInterface)
}

// main is an entry function for this package.
func Entry() {
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	//selfTest()
	server := rpc.NewServer()
	rankService := rankservice.NewLocalRankService()
	registerRankService(server, rankService)
	addr := ":20172"
	log.Printf("Listening %v for rank service...", addr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", addr)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	db := dbservice.New(":20181")
	var getAllUserRatingsReply dbservice.GetAllUserRatingsReply
	err := db.GetAllUserRatings(&dbservice.GetAllUserRatingsRequest{}, &getAllUserRatingsReply)
	if err != nil {
		log.Fatalf("GetAllUserRatings error: %v", err.Error())
	} else {
		for i, id := range getAllUserRatingsReply.Id {
			rankService.Rank.SetWithNickname(id, getAllUserRatingsReply.Rating[i], getAllUserRatingsReply.Nickname[i])
		}
		server.Accept(l)
	}
}
