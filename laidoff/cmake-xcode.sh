#!/bin/bash
mkdir build-xcode
cd build-xcode
cmake -G Xcode -DPOLLER=kqueue ..

