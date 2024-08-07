package pushservice

import (
	"puck-server/match-server/rpchelper"
	"puck-server/shared-server"
)

type Context struct {
	rpcContext *rpchelper.Context
}

func (c *Context) RegisterPushToken(args *shared_server.PushToken, reply *int) error {
	return c.rpcContext.Call("RegisterPushToken", args, reply)
}

func (c *Context) Broadcast(args *shared_server.BroadcastPush, reply *int) error {
	return c.rpcContext.Call("Broadcast", args, reply)
}

func New(address string) shared_server.PushService {
	c := new(Context)
	c.rpcContext = rpchelper.New("PushService", address)
	return c
}
