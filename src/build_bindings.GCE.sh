#!/bin/bash

#requires sudo apt-get install libboost-all-dev
#might also need sudo apt-get install python3-dev

#CXX="clang++" ; thingy="-install_name"
CXX="g++" ; thingy="-soname"

WARN='-pedantic -Wall -Wextra -Wshadow -Wunused -Wuninitialized'

name="gcn_model_for_nn"

#debug=""
#debug="-g"
debug="-D_GLIBCXX_DEBUG"

$CXX -c -fPIC $name.cc -o ${name}.o $WARN -O3 -std=c++17 -I/usr/include/python3.8 $debug

#$CXX -L /usr/lib/x86_64-linux-gnu $WARN -fPIC -shared -Wl,${thingy},${name}.so -O3 -std=c++17 -o ${name}.cpython-38-x86_64-linux-gnu.so ${name}.o -I/usr/include/python3.8m -lboost_system -lboost_python38 -lpython3.7m -lboost_numpy3 $debug

$CXX -L /usr/lib/x86_64-linux-gnu $WARN -fPIC -shared -Wl,-soname,${name}.so -O3 -std=c++17 -o ${name}.cpython-38-x86_64-linux-gnu.so ${name}.o -I/usr/include/python3.8 -lboost_system -lboost_python38 -lpython3.8 -lboost_numpy38 $debug
