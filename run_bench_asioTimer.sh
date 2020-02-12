#!/bin/bash
set -e

g++ \
-g \
-O3 \
-fno-omit-frame-pointer \
boostTimerBenchmark.cpp \
-lboost_system \
-lbenchmark \
-lpthread \
-o bench_asioTimer \
$1

sudo cpupower frequency-set --governor performance > /dev/null

./bench_asioTimer

sudo cpupower frequency-set --governor powersave > /dev/null
