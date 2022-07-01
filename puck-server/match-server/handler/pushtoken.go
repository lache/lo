package handler

import (
	"bytes"
	"fmt"
	"log"
	"net"
	"puck-server/match-server/convert"
	"puck-server/match-server/service"
	"puck-server/shared-server"
)

func HandlePushToken(buf []byte, conn net.Conn, serviceList *service.List) {
	log.Printf("PUSHTOKEN received")
	// Parse
	recvPacket, err := convert.ParsePushToken(buf)
	if err != nil {
		log.Printf("HandlePushToken fail: %v", err.Error())
	}
	idBytes := convert.IdCuintToByteArray(recvPacket.Id)
	pushTokenBytes := convert.PushTokenBytes(recvPacket) // C.GoBytes(unsafe.Pointer(&recvPacket.Push_token), C.LW_PUSH_TOKEN_LENGTH)
	pushTokenLength := bytes.IndexByte(pushTokenBytes, 0)
	pushToken := string(pushTokenBytes[:pushTokenLength])
	log.Printf("Push token domain %v, token: %v, id: %v", recvPacket.Domain, pushToken, idBytes)
	var pushResult int
	err = serviceList.Push.RegisterPushToken(&shared_server.PushToken{
		Domain:    int(recvPacket.Domain),
		PushToken: pushToken,
		UserId:    idBytes,
	}, &pushResult)
	if err != nil {
		log.Printf("RegisterPushToken returned error: %v", err.Error())
	} else {
		log.Printf("Push result: %v", pushResult)
	}
	if pushResult == 1 {
		sysMsg := []byte(fmt.Sprintf("토큰 등록 완료! %v", pushToken))
		queueOkBuf := convert.Packet2Buf(convert.NewLwpSysMsg(sysMsg))
		conn.Write(queueOkBuf)
	} else {
		sysMsg := []byte(fmt.Sprintf("토큰 등록 실패 - 서버 오류"))
		queueOkBuf := convert.Packet2Buf(convert.NewLwpSysMsg(sysMsg))
		conn.Write(queueOkBuf)
	}
}
