#!/bin/bash
set -e

g++ \
-I /home/marko/work/EPICS/timerQueue/epics-base/include \
-I /home/marko/work/EPICS/timerQueue/epics-base/include/compiler/gcc/ \
-I /home/marko/work/EPICS/timerQueue/epics-base/include/os/Linux/ \
timerQueueBenchmark.cpp \
-g \
-O3 \
-fno-omit-frame-pointer \
-L /home/marko/work/EPICS/timerQueue/epics-base/lib/linux-x86_64-debug/ \
-l Com \
-lbenchmark \
-pthread \
-o bench_epics_timerqueue \
$1

sudo cpupower frequency-set --governor performance > /dev/null

./bench_epics_timerqueue

sudo cpupower frequency-set --governor powersave > /dev/null
