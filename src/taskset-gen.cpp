/*
 * @file taskset-gen.cpp
 * @brief Taskset generation implementation (UUniFast-discard)
 * @author Sandeep D'souza 
 * 
 * Copyright (c) Carnegie Mellon University, 2018. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 	1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Include Stdlib Headers
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

// Include internal headers
#include "taskset-gen.hpp"

/**************** The UUniFast-Discard Algorithm to generate taskset utilization numbers ********************/ 
/* Params: number_tasks: number of tasks
           utilization bound: taskset utilization desired
           task_upper_bound: the upper bound on a single task's utilization
           utilization_array: vector of generated utilization values */
int UUniFast(int number_tasks, double utilization_bound, double task_upper_bound, std::vector<double> &utilization_array)
{
	double sum;
	double next_sum;
	int i;
	double random;
	int found_flag;
	int unifast_iterations = 0;
	int terminate_iterations = 1000;	// Iterations to terminate UUniFast-Discard if any task has util > 1

	// Return -1 if the number of tasks are too less
	if (utilization_bound/number_tasks > task_upper_bound)
		return -1;

	for(unifast_iterations = 0; unifast_iterations< terminate_iterations; unifast_iterations++)
	{ 
		sum = utilization_bound;
		found_flag = 1;
		// Generate task utilization values
		for(i=1; i<number_tasks; i++)
		{
			random = (double)(rand() % 10000000)/(double)10000000;
			next_sum = sum*((double)pow(random, ((double)1/((double)(number_tasks - i)))));
			utilization_array[i-1] = sum - next_sum;
			if(utilization_array[i-1] > task_upper_bound)
			{
				found_flag = 0;
				break;
			}
			sum = next_sum;
		}
		// If a feasible taskset is found, generate the last task
		if(found_flag == 1)
		{
			utilization_array[i-1] = sum;
			if(utilization_array[i-1] <= task_upper_bound)
			{
				break;
			}
			else
			{
				found_flag = 0;
			}
		}
	}
	return -(1-found_flag);		// Returns 0 if found, -1 if not
}

/**************** Generate a random taskset ********************/ 
/* Params: number_gpu_tasks: number of tasks with gpu sections
		   max_gpu_segments: maximum GPU segments per task 
		   random_flag: if 1 set randomly, if not set to value
		   per_task_gpu_segments: vector of gpu task segments per task */
int generate_random_num_gpu_segments(int number_gpu_tasks, int max_gpu_segments, int random_flag, std::vector<int> &per_task_gpu_segments)
{
	int i, random, total_segments = 0;
	for (i = 0; i < per_task_gpu_segments.size(); i++)
	{
		random = rand();
		if (max_gpu_segments > 1 && random_flag)
			per_task_gpu_segments[i] = (random % (max_gpu_segments - 1)) + 1;
		else if (max_gpu_segments > 1)
			per_task_gpu_segments[i] = max_gpu_segments;
		else
			per_task_gpu_segments[i] = 1;
		total_segments = total_segments + per_task_gpu_segments[i];
	}
	return total_segments;
}

/**************** Generate a random taskset ********************/ 
/* Params: number_tasks: number of tasks
		   number_gpu_tasks: number of tasks with gpu sections
		   max_gpu_segments: maximum GPU segments per task
           utilization bound: taskset utilization desired
           gpu_utilization_bound: taskset utilization gpu bound 
           harmonic_flag: true indicates harmonic period
           gpu_seg_random_flag: true indicates generate number of per task gpu segments randomly using max
   		   max_gpu_fraction: maximum fraction of the GPU that a gpu request consumes
   Returns: Vector of Tasks, empty vector in case of error */
std::vector<Task> generate_tasks(int number_tasks, int number_gpu_tasks, int max_gpu_segments, double utilization_bound, double gpu_utilization_bound, int harmonic_flag, int gpu_seg_random_flag, double max_gpu_fraction)
{
	int min_period = MIN_PERIOD;					        // in ms
	int max_period = MAX_PERIOD;							// in ms
	double cpu_task_upper_bound = CPU_TASK_UPPER_BOUND;  
	double gpu_task_upper_bound = GPU_TASK_UPPER_BOUND;  
	double cpu_intervention_util = CPU_INTERVENTION_UTIL;
	double cpu_intervention_bound = CPU_INTERVENTION_BOUND;         
           
	int i = 0;
	int random;
	gpu_params_t G;

	int total_gpu_segments; // total number of GPU segments
	std::vector<int> per_task_gpu_segments(number_gpu_tasks, 1);

	// Get the total number of GPU segments
	total_gpu_segments = generate_random_num_gpu_segments(number_gpu_tasks, max_gpu_segments, gpu_seg_random_flag, per_task_gpu_segments);

	// Generate utilization arrays
	std::vector<double> utilization_array(number_tasks, 0.0);
	std::vector<double> gpu_utilization_array(total_gpu_segments, 0.0);
	std::vector<Task> task_vector;
	task_t task_params;

	// Check if number of tasks with GPU sections is less than total number of tasks
	if (number_gpu_tasks > number_tasks || number_tasks <= 0)
		return task_vector;
	
	// Generate CPU utilization array using UUniFast-Discard
	if(UUniFast(number_tasks, utilization_bound, cpu_task_upper_bound, utilization_array))
	{
		return task_vector;
	}

	if (number_gpu_tasks > 0)
	{
		// Generate GPU utilization array using UUniFast-Discard
		if(UUniFast(total_gpu_segments, gpu_utilization_bound, gpu_task_upper_bound, gpu_utilization_array))
		{
			return task_vector;
		}
	}

	while(i < number_tasks)
	{
		// Randomly Initialize Time Periods
		random = rand();
		if(harmonic_flag == 1 && i == 0)
		{
			task_params.T = (random % (min_period)) + min_period;
		}
		else if(harmonic_flag == 1 && i > 0)
		{
			task_params.T = ((random % (3)) + 1)*task_params.T;
		}
		else
		{
			task_params.T = (random % (max_period-min_period)) + min_period;
		}

		// Set the deadline to be implicit
		task_params.D = task_params.T;

		// Set the CPU computation time based on UUniFast
		task_params.C = utilization_array[i]*task_params.T;

		// Clear the gpu param vector
		task_params.G.clear();

		// Set the GPU computation time
		if (i < number_gpu_tasks)
		{
			// Set the parameters
			for (int j = 0; j < per_task_gpu_segments[i]; j++)
			{
				G.Ge = gpu_utilization_array[j]*task_params.T;
				if (cpu_intervention_util*G.Ge < cpu_intervention_bound)
				{
					G.Gm = cpu_intervention_util*G.Ge;
					G.Ge = G.Ge - G.Gm;
				}
				else
				{
					G.Gm = cpu_intervention_bound;
					G.Ge = G.Ge - cpu_intervention_bound;
				}
				random = rand();
				G.F = ((double) ((random % (GPU_FRACTION_GRANULARITY - 1)) + 1))/GPU_FRACTION_GRANULARITY;
				// Floor the fraction at the max gpu fraction
				if (G.F > max_gpu_fraction)
					G.F = max_gpu_fraction;

				task_params.G.push_back(G);
			}
			
		}

		// Create the task class
		Task task(task_params);
		task_vector.push_back(task);

		i++;
	}
	return task_vector;
}