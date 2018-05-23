package main

import (
	"testing"
	"github.com/lache/lo/laidoff/match-server/service"
	"github.com/lache/lo/laidoff/rank-server/rankservice"
	"github.com/lache/lo/laidoff/db-server/user"
	"time"
	"github.com/stretchr/testify/assert"
	"log"
	"os"
)

func queueScoreMatch(id byte, score int, serviceList *service.List, distanceByElapsed *rankservice.DistanceByElapsed) (rankservice.QueueScoreMatchReply, error) {
	return queueScoreMatchUpdate(id, score, serviceList, distanceByElapsed, false)
}

func queueScoreMatchUpdate(id byte, score int, serviceList *service.List, distanceByElapsed *rankservice.DistanceByElapsed, update bool) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		Id:                user.Id{id},
		Score:             score,
		DistanceByElapsed: *distanceByElapsed,
		Update:            update,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func flushQueueScoreMatch(serviceList *service.List) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		Flush: true,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func setMatchPoolTimeBias(serviceList *service.List, bias time.Duration) (rankservice.QueueScoreMatchReply, error) {
	request := rankservice.QueueScoreMatchRequest{
		SetBias:           true,
		MatchPoolTimeBias: bias,
	}
	var reply rankservice.QueueScoreMatchReply
	err := serviceList.Rank.QueueScoreMatch(&request, &reply)
	return reply, err
}

func assertNotMatchedFirstQueue(t *testing.T, reply rankservice.QueueScoreMatchReply, err error) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.Nil(t, reply.RemoveNearestOverlapResult)
}

func assertNotMatched(t *testing.T, reply rankservice.QueueScoreMatchReply, err error, nearestId byte) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.NotNil(t, reply.RemoveNearestOverlapResult)
	assert.Equal(t, false, reply.RemoveNearestOverlapResult.Matched, "Matched")
	assert.Equal(t, user.Id{nearestId}, reply.RemoveNearestOverlapResult.NearestResult.NearestId)
}

func assertMatched(t *testing.T, reply rankservice.QueueScoreMatchReply, err error, matchedId byte) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.NotNil(t, reply.RemoveNearestOverlapResult)
	assert.Equal(t, true, reply.RemoveNearestOverlapResult.Matched, "Matched")
	assert.Equal(t, user.Id{matchedId}, reply.RemoveNearestOverlapResult.NearestResult.NearestId)
}

func assertAlreadyRemoved(t *testing.T, reply rankservice.QueueScoreMatchReply, err error) {
	assert.Equal(t, nil, err)
	assert.Equal(t, nil, reply.Err)
	assert.NotNil(t, reply.RemoveNearestOverlapResult)
	assert.Equal(t, true, reply.RemoveNearestOverlapResult.AlreadyRemoved)
}

func createServiceListAndDistanceByElapsed(t *testing.T) (*service.List, *rankservice.DistanceByElapsed) {
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	serviceList := service.NewServiceListLocalTest()
	//serviceList := service.NewServiceList()
	distanceByElapsed := rankservice.DistanceByElapsed{
		Elapsed: []time.Duration{
			30 * time.Second,
			20 * time.Second,
			10 * time.Second,
			0 * time.Second},
		Distance: []int{
			100,
			50,
			25,
			5},
	}
	// Flush queue (for easy debugging)
	_, err := flushQueueScoreMatch(serviceList)
	setMatchPoolTimeBias(serviceList, 0)
	assert.Equal(t, nil, err)
	return serviceList, &distanceByElapsed
}

func TestMatchRpc_Case1(t *testing.T) {
	serviceList, d := createServiceListAndDistanceByElapsed(t)
	// User 1
	reply1, err := queueScoreMatch(1, 100, serviceList, d)
	assertNotMatchedFirstQueue(t, reply1, err)
	// User 2
	reply2, err := queueScoreMatch(2, 99, serviceList, d)
	assertMatched(t, reply2, err, 1)
}

func TestMatchRpc_Case2(t *testing.T) {
	serviceList, d := createServiceListAndDistanceByElapsed(t)
	// User 1
	reply1, err := queueScoreMatch(1, 100, serviceList, d)
	assertNotMatchedFirstQueue(t, reply1, err)
	// User 2 (first try)
	reply2a, err := queueScoreMatch(2, 80, serviceList, d)
	assertNotMatched(t, reply2a, err, 1)
	// Delay goes here...
	setMatchPoolTimeBias(serviceList, 15*time.Second)
	// User 2 (second try)
	reply2b, err := queueScoreMatch(2, 80, serviceList, d)
	assertMatched(t, reply2b, err, 1)
}

func TestMatchRpc_Case3(t *testing.T) {
	serviceList, d := createServiceListAndDistanceByElapsed(t)
	// User 1
	reply1, err := queueScoreMatch(1, 100, serviceList, d)
	assertNotMatchedFirstQueue(t, reply1, err)
	// User 2
	reply2, err := queueScoreMatch(2, 80, serviceList, d)
	assertNotMatched(t, reply2, err, 1)
	// User 3
	reply3, err := queueScoreMatch(3, 80, serviceList, d)
	assertMatched(t, reply3, err, 2)
	// User 4
	reply4, err := queueScoreMatch(4, 100, serviceList, d)
	assertMatched(t, reply4, err, 1)
}

func TestMatchRpc_Case4(t *testing.T) {
	serviceList, d := createServiceListAndDistanceByElapsed(t)
	// User 1
	reply1, err := queueScoreMatch(1, 100, serviceList, d)
	assertNotMatchedFirstQueue(t, reply1, err)
	// User 2
	reply2, err := queueScoreMatch(2, 100, serviceList, d)
	assertMatched(t, reply2, err, 1)
	// User 1 [update]
	reply3, err := queueScoreMatchUpdate(1, 100, serviceList, d, true)
	assertAlreadyRemoved(t, reply3, err)
	// User 2 [update]
	reply4, err := queueScoreMatchUpdate(2, 100, serviceList, d, true)
	assertAlreadyRemoved(t, reply4, err)
}
