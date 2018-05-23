#!/bin/sh

cd services/db
../../bin/br-server db >> db.log 2>&1 &
DB_PID=$!
cd ../..
sleep 1

cd services/match
../../bin/br-server match >> match.log 2>&1 &
MATCH_PID=$!
cd ../..
sleep 1

cd services/rank
../../bin/br-server rank >> rank.log 2>&1 &
RANK_PID=$!
cd ../..
sleep 1

cd services/reward
../../bin/br-server reward >> reward.log 2>&1 &
REWARD_PID=$!
cd ../..
sleep 1

cd services/push
../../bin/br-server push >> push.log 2>&1 &
PUSH_PID=$!
cd ../..
sleep 1

cd services/battle
../../bin/laidoff-server >> laidoff.log 2>&1
# Never reach here...
LAIDOFF_PID=$!


