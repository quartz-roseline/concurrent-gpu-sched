/*
 * @file taskset.cpp
 * @brief Taskset and Task operations helper functions
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

#include <iostream> 
#include <cmath>

#include "taskset.hpp"

/**************** Print all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: 0 if schedulable */
void print_taskset(const std::vector<Task> &task_vector)
{
	int num_gpu_segments;
	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		num_gpu_segments = task_vector[index].getNumGPUSegments();
		std::cout << "Task " << index << 
		             ": C = " << task_vector[index].getC() <<
		             ", D = " << task_vector[index].getD() <<
		             ", T = " << task_vector[index].getT() << 
		             ", No. GPU segments = " << num_gpu_segments <<
		             ", Core = " << task_vector[index].getCoreID() <<"\n";
		for (unsigned int j = 0; j < num_gpu_segments; j++)
		{
			std::cout << "GPU Segment " << j << 
						 ", Ge = " << task_vector[index].getGe(j) <<
		                 ", Gm = " << task_vector[index].getGm(j) <<
		                 ", F  = " << task_vector[index].getF(j) << "\n";
		}
	}
}

/**************** Scale the task set CPU & GPU frequency by the given factor ********************/ 
/* Params: task_vector: vector of tasks 
		   cpu_frequency: cpu frequency scaling factor
		   gpu_frequency: gpu frequency scaling factor
   Returns: new task vector */
std::vector<Task> scale_taskset_frequency(const std::vector<Task> &task_vector, double cpu_frequency, double gpu_frequency)
{
	std::vector<Task> scaled_task_vector;

	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		Task task = task_vector[index];
		task.scale_cpu(cpu_frequency);
		task.scale_gpu(gpu_frequency);
		scaled_task_vector.push_back(task);
	}
	return scaled_task_vector;
}

/**************** Get the CPU utilization all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: cpu utilization */
double get_taskset_cpu_util(const std::vector<Task> &task_vector)
{
	double cpu_util = 0.00;
	int num_gpu_segments;
	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		num_gpu_segments = task_vector[index].getNumGPUSegments();
		cpu_util = cpu_util + ((task_vector[index].getC())/task_vector[index].getT());
		for (unsigned int j = 0; j < num_gpu_segments; j++)
		{
			cpu_util = cpu_util + ((task_vector[index].getGm(j))/task_vector[index].getT());
		}
	}
	return cpu_util;
}

/**************** Get the CPU utilization GPU-using tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: cpu utilization of GPU-using tasks */
double get_gputasks_cpu_util(const std::vector<Task> &task_vector)
{
	double cpu_util = 0.00;
	int num_gpu_segments;
	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		num_gpu_segments = task_vector[index].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			cpu_util = cpu_util + ((task_vector[index].getC())/task_vector[index].getT());
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				cpu_util = cpu_util + ((task_vector[index].getGm(j))/task_vector[index].getT());
			}
		}
	}
	return cpu_util;
}

/**************** Get the GPU utilization all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: gpu utilization */
double get_taskset_gpu_util(const std::vector<Task> &task_vector)
{
	double gpu_util = 0.00;
	int num_gpu_segments;
	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		num_gpu_segments = task_vector[index].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				gpu_util = gpu_util + ((task_vector[index].getGe(j))/task_vector[index].getT());
			}
		}
	}
	return gpu_util;
}

/* Find index of max lp task with largest GPU segment */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: index of low-priority task with max GPU segment */
int find_max_lp_gpu_index(int index, const std::vector<Task> &task_vector)
{
	int Gl_max_index = index + 1;
	double Gl_max = 0;
	double Gl;
	int num_gpu_segments;
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				Gl = task_vector[i].getGe(j) + task_vector[i].getGm(j);
				if (Gl > Gl_max)
				{
					Gl_max = Gl;
					Gl_max_index = i;
				}
			}
		}
	}
	return Gl_max_index;
}

/* Find the length of the lp task with largest GPU segment */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: length of the max low-priority task GPU segment */
double find_max_lp_gpu_segment(int index, const std::vector<Task> &task_vector)
{
	int Gl_max_index = index + 1;
	double Gl_max = 0;
	double Gl;
	int num_gpu_segments;
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				Gl = task_vector[i].getGe(j) + task_vector[i].getGm(j);
				if (Gl > Gl_max)
				{
					Gl_max = Gl;
					Gl_max_index = i;
				}
			}
		}
	}
	return Gl_max;
}

/* Find tindex of max lp task with largest GPU segment in terms of WCRT */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: index of low-priority task with max WCRT low-priority task GPU segment */
int find_max_lp_gpu_wcrt_index(int index, const std::vector<Task> &task_vector)
{
	int H_max_index = index + 1;
	double H_max = 0;
	double H;
	int num_gpu_segments;
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				H = task_vector[i].getH(j);
				if (H > H_max)
				{
					H_max = H;
					H_max_index = i;
				}
			}
		}
	}
	return H_max_index;
}

/* Find the length of the lp task with largest GPU segment in terms of WCRT */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_max_lp_gpu_wcrt_segment(int index, const std::vector<Task> &task_vector)
{
	int H_max_index = index + 1;
	double H_max = 0;
	double H;
	int num_gpu_segments;
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				H = task_vector[i].getH(j);
				if (H > H_max)
				{
					H_max = H;
					H_max_index = i;
				}
			}
		}
	}
	return H_max;
}

/* Find the length of the lp task with largest GPU segment in terms of WCRT, which is smaller than value */
/* Params: index      : task index in vector ordered by priority
		   value      : value which wcrt should be less than or equal to
		   lp_index   : the index of the said task (populated by this function)
		   num_biggest: the kth largest element to find (starting at 1st)
           task_vector: vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_next_max_lp_gpu_wcrt_segment(int index, double value, int &lp_index, int num_biggest, const std::vector<Task> &task_vector)
{
	int H_max_index = index + 1;
	double H_max = 0;
	double H;
	int num_gpu_segments;
	int counter = 0;
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				H = task_vector[i].getH(j);
				if (H > H_max && H < value)
				{
					H_max = H;
					H_max_index = i;
				}

				// Logic to take care of cases where we have 2 or more similar numbers
				if (H >= value)
				{
					counter++;
				}

				if (counter == num_biggest)
				{
					H_max = H;
					H_max_index = i;
					break;
				}
			}
		}
	}
	lp_index = H_max_index;
	return H_max;
}

/* Find the length of the lp task with largest GPU segment in terms of WCRT, which is smaller than value and has larger fraction */
/* Params: index       : task index in vector ordered by priority
		   value       : value which wcrt should be less than or equal to
		   num_biggest : the kth largest element to find (starting at 1st)
		   req_fraction: the fraction of the chosen request (populated by this function)
		   fraction    : smallest fraction request to consider
           task_vector : vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_next_max_lp_gpu_wcrt_segment_frac(int index, double value, int num_biggest, double &req_fraction, 
											  double fraction, const std::vector<Task> &task_vector)
{
	int H_max_index = index + 1;
	double H_max = 0;
	double H;
	int num_gpu_segments;
	int counter = 0;

	// Initialize the chosen request fraction to 0
	req_fraction = 0;
	
	// Find the largest request meeting the criteria
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (num_gpu_segments != 0)
		{
			for (unsigned int j = 0; j < num_gpu_segments; j++)
			{
				// Ignore if the fraction is smaller than the stated fraction 
				if (task_vector[i].getF(j) < fraction)
					continue;

				H = task_vector[i].getH(j);
				if (H > H_max && H < value)
				{
					H_max = H;
					H_max_index = i;
					req_fraction = task_vector[i].getF(j);
				}

				// Logic to take care of cases where we have 2 or more similar numbers
				if (H >= value)
				{
					counter++;
				}

				if (counter == num_biggest)
				{
					H_max = H;
					H_max_index = i;
					req_fraction = task_vector[i].getF(j);
					break;
				}
			}
		}
	}
	return H_max;
}

/* Find the kth largest cpu intervention of a lp task which is smaller (<=) than value */
/* Params: index      : task index in vector ordered by priority which we need to search
		   value      : value which wcrt should be less than or equal to
		   num_biggest: the kth largest element to find (starting at 1st)
           task_vector: vector of tasks 
   Returns: length of the kth max cpu intervention segment of a low-priority task GPU segment */
double find_next_task_max_gpu_intervention_segment(int index, double value, int num_biggest, const std::vector<Task> &task_vector)
{
	double Gm_max = 0;
	double Gm;
	int num_gpu_segments;
	int counter = 0;
	num_gpu_segments = task_vector[index].getNumGPUSegments();
	if (num_gpu_segments != 0)
	{
		for (unsigned int j = 0; j < num_gpu_segments; j++)
		{
			Gm = task_vector[index].getGm(j);
			if (Gm > Gm_max && Gm < value)
			{
				Gm_max = Gm;
			}

			// Logic to take care of cases where we have 2 or more similar numbers
			if (Gm >= value)
			{
				counter++;
			}

			if (counter == num_biggest)
			{
				Gm_max = Gm;
				break;
			}
		}
	}
	return Gm_max;
}

/**************** Calculate upper bound on number of instance of low-prio tasks in the response time ********************/ 
/* Params: lp_task       : the low-prio task class we want to operate on
		   response_time : the time in which we want to see how many effective number of instance show up
   Returns: effective number of instances of the low-prio task which show up */
unsigned int getTheta(const Task &lp_task, double response_time)
{
	unsigned int theta = 0;
	theta = ceil((response_time + lp_task.getD() - lp_task.getE())/lp_task.getT());
	return theta;
}
