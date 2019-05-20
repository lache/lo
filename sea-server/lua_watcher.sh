#!/bin/bash

fswatch -o assets/l | xargs -n1 lua assets/l/run_tests.lua

