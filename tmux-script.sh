#!/bin/bash

session="ball-rumble"

tmux new-session -d -s $session

window=0
tmux rename-window -t $session:$window 'db'
tmux send-keys -t $session:$window 'cd ~/lo/puck-server/db-server' C-m
tmux send-keys -t $session:$window '../br db' C-m

window=1
tmux new-window -t $session:$window -n 'match'
tmux send-keys -t $session:$window 'cd ~/lo/puck-server/match-server' C-m
tmux send-keys -t $session:$window '../br match' C-m

window=2
tmux new-window -t $session:$window -n 'push'
tmux send-keys -t $session:$window 'cd ~/lo/puck-server/push-server' C-m
tmux send-keys -t $session:$window '../br push' C-m

window=3
tmux new-window -t $session:$window -n 'rank'
tmux send-keys -t $session:$window 'cd ~/lo/puck-server/rank-server' C-m
tmux send-keys -t $session:$window '../br rank' C-m

window=4
tmux new-window -t $session:$window -n 'reward'
tmux send-keys -t $session:$window 'cd ~/lo/puck-server/reward-server' C-m
tmux send-keys -t $session:$window '../br reward' C-m

window=5
tmux new-window -t $session:$window -n 'battle'
tmux send-keys -t $session:$window 'cd ~/lo/laidoff/build-server' C-m
tmux send-keys -t $session:$window './bin/laidoff-server' C-m

