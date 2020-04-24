#!/bin/bash
set -e

echo "EPICS_BASE_DIR: ${EPICS_BASE_DIR}"
git -C ${EPICS_BASE_DIR} rev-parse HEAD

g++ \
-I ${EPICS_BASE_DIR}/include \
-I ${EPICS_BASE_DIR}/include/compiler/gcc/ \
-I ${EPICS_BASE_DIR}/include/os/Linux/ \
timerQueueBenchmark.cpp \
-g \
-O3 \
-fno-omit-frame-pointer \
-L ${EPICS_BASE_DIR}/lib/linux-x86_64/ \
-lCom \
-lbenchmark \
-pthread \
-o bench_epics_timerqueue \
$1

sudo cpupower frequency-set --governor performance > /dev/null

LD_LIBRARY_PATH=${EPICS_BASE_DIR}/lib/linux-x86_64 ./bench_epics_timerqueue

sudo cpupower frequency-set --governor powersave > /dev/null
