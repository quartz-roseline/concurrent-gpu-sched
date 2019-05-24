#!/bin/bash
# Sweep over CPU utilization
NUM_TASKSETS=5000
HARMONIC_FLAG=0
FILENAME=$1 #mcproc_cpu_util_sweep_.csv
EPSILON=0.01
GPU_UTIL=0.3
GPU_TASK_FRACTION=0.5
MAX_GPU_SEGMENTS=10
MAX_NUM_TASKS=15
MAX_GPU_FRACTION=$2 #0.3 0.5 0.7 1.0
MODE=0
NUM_CORES=6 #4
#for cpu_util in {40..300..10} # for 4 cores
#for cpu_util in {300..600..20} # for 8 cores
for cpu_util in {200..500..20} # for 6 cores
do
	CPU_UTIL=$(echo "scale = 1; $cpu_util/100" | bc)
	echo $CPU_UTIL
	../src/mcprocessor_exp.out $NUM_TASKSETS $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL $GPU_TASK_FRACTION $MAX_GPU_SEGMENTS $MAX_NUM_TASKS $MAX_GPU_FRACTION $MODE $NUM_CORES
done
exit 0