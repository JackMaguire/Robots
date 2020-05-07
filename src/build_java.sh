#!/bin/bash

javac RobotsFrontend.java

g++ robots_backend.cc -std=c++2a -o robots_backend -Wall -pedantic -Wshadow -g -D_GLIBCXX_DEBUG

