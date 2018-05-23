package shared_server

type Arith interface {
	Multiply(args *Args, reply *int) error
	Divide(args *Args, quo *Quotient) error
}

type PushService interface {
	RegisterPushToken(args *PushToken, reply *int) error
	Broadcast(args *BroadcastPush, reply *int) error
}
