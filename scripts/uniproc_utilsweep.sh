#!/bin/bash
# Sweep over CPU utilization
NUM_TASKSETS=5000
HARMONIC_FLAG=0
FILENAME=$1 #cpu_util_sweep_.csv
EPSILON=0.01
GPU_UTIL=0.3
GPU_TASK_FRACTION=0.5
MAX_GPU_SEGMENTS=10
MAX_NUM_TASKS=10
MAX_GPU_FRACTION=$2 #0.3 0.5 0.7 1.0
MODE=0
for cpu_util in {10..70..10}
do
	CPU_UTIL=$(echo "scale = 1; $cpu_util/100" | bc)
	echo $CPU_UTIL
	../src/uniprocessor_exp.out $NUM_TASKSETS $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL $GPU_TASK_FRACTION $MAX_GPU_SEGMENTS $MAX_NUM_TASKS $MAX_GPU_FRACTION $MODE
done
exit 0