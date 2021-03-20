#!/bin/bash

#find ../wt/build/src/ | grep "\.so" | xargs -n1 ln -s
#ln -s ../wt/build/src/http/libwthttp.so libwthttp2.so
#ln -s ../wt/build/src/libwt.so libwt2
#ln -s ../wt/resources/

WARN="-Wall -Wshadow -Wunused -pedantic -Wextra -Werror"
WARN="$WARN -Wno-subobject-linkage"

#DEBUG="-D_GLIBCXX_DEBUG"
DEBUG="$DEBUG -g"

opt="-O3"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/tf/lib/

g++-8 -std=c++17 -o robots_app robots_app.cc $opt -lwthttp -lwt -ltensorflow -lboost_signals -isystem wt_src -isystem wt_build_src -isystem cppflow/include -isystem tf/include -L. -Ltf/lib -lstdc++fs -Wl,-rpath,.  $WARN $DEBUG && \
    ./robots_app --docroot . --http-address 0.0.0.0 --http-port 8080
