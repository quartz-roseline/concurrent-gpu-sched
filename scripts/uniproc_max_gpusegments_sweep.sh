#!/bin/bash
# Sweep over max number of GPU segments in the task
NUM_TASKSETS=5000
HARMONIC_FLAG=0
FILENAME=$1	#gpu_segment_sweep_.csv
EPSILON=0.01
CPU_UTIL=0.4
GPU_UTIL=0.5
GPU_TASK_FRACTION=0.5
MAX_NUM_TASKS=10
MAX_GPU_FRACTION=$2 #0.3 0.5 0.7 1.0
MODE=2
for gpu_segments in {1..12..2}
do
	MAX_GPU_SEGMENTS=$(echo "scale = 1; $gpu_segments" | bc)
	echo $MAX_GPU_SEGMENTS
	../src/uniprocessor_exp.out $NUM_TASKSETS $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL $GPU_TASK_FRACTION $MAX_GPU_SEGMENTS $MAX_NUM_TASKS $MAX_GPU_FRACTION $MODE
done
exit 0