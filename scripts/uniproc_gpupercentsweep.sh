#!/bin/bash
# Sweep over percentage of tasks which have GPU segments
NUM_TASKSETS=5000
HARMONIC_FLAG=0
FILENAME=$1	#gpu_percent_sweep_.csv
EPSILON=0.01
CPU_UTIL=0.4
GPU_UTIL=0.5
MAX_GPU_SEGMENTS=10
MAX_NUM_TASKS=10
MAX_GPU_FRACTION=$2 #0.3 0.5 0.7 1.0
MODE=1
for gpu_percent in {0..70..10}
do
	GPU_TASK_FRACTION=$(echo "scale = 1; $gpu_percent/100" | bc)
	echo $GPU_TASK_FRACTION
	../src/uniprocessor_exp.out $NUM_TASKSETS $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL $GPU_TASK_FRACTION $MAX_GPU_SEGMENTS $MAX_NUM_TASKS $MAX_GPU_FRACTION $MODE
done
exit 0