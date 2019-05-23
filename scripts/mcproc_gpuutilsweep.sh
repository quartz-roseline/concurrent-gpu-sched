#!/bin/bash
NUM_TASKS=5000
HARMONIC_FLAG=0
FILENAME="mcgpu_util_sweep.txt"
EPSILON=0.01
CPU_UTIL=1.5
NUM_CORES=4
for util in {10..70..10}
do
GPU_UTIL=$(echo "scale = 1; $util/100" | bc)
echo $GPU_UTIL
../src/mcprocessor_exp $NUM_TASKS $NUM_CORES $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL 
done
exit 0