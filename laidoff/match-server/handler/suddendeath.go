package handler

import (
	"log"
	"net"
	"github.com/gasbank/laidoff/match-server/config"
)

func HandleSuddenDeath(conf config.ServerConfig, buf []byte) {
	log.Printf("SUDDENDEATH received")
	tcpAddr, err := net.ResolveTCPAddr(conf.BattleServiceConnType, conf.BattleServiceHost+":"+conf.BattleServicePort)
	if err != nil {
		log.Printf("ResolveTCPAddr error! - %v", err.Error())
	}
	conn, err := net.DialTCP("tcp", nil, tcpAddr)
	if err != nil {
		log.Printf("DialTCP error! - %v", err.Error())
	}
	_, err = conn.Write(buf)
	if err != nil {
		log.Printf("Send SUDDENDEATH failed")
	}
}
