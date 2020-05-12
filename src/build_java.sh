#!/bin/bash

javac RobotsFrontend.java

g++ robots_backend.cc -std=c++2a -o robots_backend -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG \
    -I ~/libtensorflow-cpu-darwin-x86_64-1.15.0/include/ -L /Users/jackmaguire/libtensorflow-cpu-darwin-x86_64-1.15.0/lib -ltensorflow
