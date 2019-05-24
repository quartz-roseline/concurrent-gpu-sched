#!/bin/bash
# Sweep over the task with the largest fractional requirement of the GPU
NUM_TASKSETS=5000
HARMONIC_FLAG=0
FILENAME=$1	#mcproc_gpu_fraction_sweep.csv
EPSILON=0.01
CPU_UTIL=1.5
GPU_UTIL=0.4
GPU_TASK_FRACTION=0.5
MAX_GPU_SEGMENTS=10
MAX_NUM_TASKS=10
MODE=3
NUM_CORES=4
for max_gpu_fraction in {10..100..10}
do
	MAX_GPU_FRACTION=$(echo "scale = 1; $max_gpu_fraction/100" | bc)
	echo $MAX_GPU_FRACTION
	../src/mcprocessor_exp.out $NUM_TASKSETS $HARMONIC_FLAG $FILENAME $EPSILON $CPU_UTIL $GPU_UTIL $GPU_TASK_FRACTION $MAX_GPU_SEGMENTS $MAX_NUM_TASKS $MAX_GPU_FRACTION $MODE $NUM_CORES
done
exit 0