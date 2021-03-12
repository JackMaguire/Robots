#!/bin/bash

#find ../wt/build/src/ | grep "\.so" | xargs -n1 ln -s
#ln -s ../wt/build/src/http/libwthttp.so libwthttp2.so
#ln -s ../wt/build/src/libwt.so libwt2
#ln -s ../wt/resources/

#WARN="-Wall -Wshadow -Wunused -pedantic -Wextra"
WARN="$WARN -Wno-subobject-linkage"

g++ -std=c++17 -o robots_app robots_app.cc -lwthttp -lwt -lboost_signals -Iwt_src -Iwt_build_src -Icppflow/include -Itf/include -L. -Ltf/lib -lstdc++fs -Wl,-rpath,. $WARN && \
    ./robots_app --docroot . --http-address 0.0.0.0 --http-port 8080
