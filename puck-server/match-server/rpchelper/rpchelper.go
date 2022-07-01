package rpchelper

import (
	"net/rpc"
	"log"
	"net"
	"errors"
	"time"
)

type Context struct {
	client *rpc.Client
	baseName string
	address string
}

func New(baseName string, address string) (*Context) {
	context := &Context{nil, baseName, address}
	return context
}

func (c *Context) Call(methodName string, args interface{}, reply interface{}) error {
	return c.callWithRetry(methodName, args, reply, 2, 300)
}

func (c *Context) callWithRetry(methodName string, args interface{}, reply interface{}, retryCount int, retryDelayMs int) error {
	if c.client == nil {
		err := c.Dial()
		if err != nil {
			log.Printf("RPC Dial error: %v", err.Error())
			time.Sleep(time.Duration(retryDelayMs) * time.Millisecond)
			if retryCount > 0 {
				return c.callWithRetry(methodName, args, reply, retryCount - 1, 2 * retryDelayMs)
			} else {
				return errors.New("rpc dial retry count exceed")
			}
		}
	}
	err := c.client.Call(c.baseName + "." + methodName, args, reply)
	if err != nil {
		log.Printf("RPC Call error: %v", err.Error())
		if err.Error() == "user db not exist" {
			// not exist err is not an rpc-related error. no retry
			return err
		}
		time.Sleep(time.Duration(retryDelayMs) * time.Millisecond)
		if retryCount > 0 {
			c.client = nil
			return c.callWithRetry(methodName, args, reply, retryCount-1, 2*retryDelayMs)
		} else {
			return errors.New("rpc call retry count exceeded")
		}
	}
	return err
}

func (c *Context) Dial() error {
	log.Printf("Dial to %v RPC server %v...", c.baseName, c.address)
	conn, err := net.Dial("tcp", c.address)
	if err != nil {
		log.Printf("Connection error: %v", err)
		return err
	} else {
		c.client = rpc.NewClient(conn)
	}
	return nil
}
