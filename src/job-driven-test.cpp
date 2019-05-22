/*
 * @file job-driven-test.cpp
 * @brief Implementing the job-driven schedulability test (for non-concurrent GPUs)
 * @author Sandeep D'souza 
 * 
 * Copyright (c) Carnegie Mellon University, 2019. All rights reserved.
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
/* Standard Library Imports */
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

/* Internal Headers */
#include "job-driven-test.hpp"
#include "indirect-cis.hpp"
#include "taskset.hpp"

/**************** Calculate Prioritized Blocking using the job-driven approach sub-routine ********************/ 
double calculate_prioritized_blocking_jd(unsigned int index, double response_time, const std::vector<Task> &task_vector)
{
	double blocking = 0;
	unsigned int theta = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	unsigned int coreID = task_vector[index].getCoreID();

	// Calculate the prioritized blocking due to low-prio tasks on the same core
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		// Tasks not on our core do not contribute
		if (task_vector[i].getCoreID() != coreID)
			continue;
		else
			theta = getTheta(task_vector[i], response_time);

		blocking = blocking + theta*task_vector[i].getTotalGm();
	}

	return blocking;
}

/**************** Calculate Per-job Direct Blocking sub-routine ********************/ 
double calculate_direct_blocking_jd(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
{
	double blocking, alpha;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();

	/* Find index of maximum low priority GPU segment */
	double Hl_max = find_max_lp_gpu_wcrt_segment(index, task_vector);	

	// Return 0 blocking if task has no GPU execution
	if(task_vector[index].getTotalGe() == 0)
		return 0;

	/* Note: Under this analysis even low-priority GPU access 
	         faces blocking from high-priority tasks */
	/* Refactor the maximum low-priority blocking to take into account the CPU frequency */
	if (Hl_max != 0)
	{
		blocking = num_gpu_segments*Hl_max;
	}
	else
	{
		// No other lower priority tasks exist -> Vector Index has overrun
		blocking = 0;
	}
	
	// Calculate the direct blocking due to all hp requests
	for (unsigned int i = 0; i < index; i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (task_vector[i].getTotalGe() != 0)
		{
			alpha = ceil((resp_time + resp_time_hp[i] - ((task_vector[i].getC()+task_vector[i].getTotalGm())))/task_vector[i].getT());
			for (unsigned int req_index = 0; req_index < num_gpu_segments; req_index++)
			{
				if (task_vector[i].getGe(req_index) != 0)
				{
					blocking = blocking + alpha*(task_vector[i].getH(req_index));
				}
			}
		}
	}
	return blocking;
}

/**************** Calculate Total Blocking sub-routine ********************/ 
double calculate_blocking_jd(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();

	// Add the prioritized blocking using the request-driven approach
	blocking = blocking + calculate_prioritized_blocking_jd(index, resp_time, task_vector);
	
	if (num_gpu_segments == 0)
		return blocking;

	// Get the direct blocking
	blocking = blocking + calculate_direct_blocking_jd(index, task_vector, resp_time_hp, resp_time);

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_jd(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
{
	double interference = 0;
	unsigned int coreID = task_vector[index].getCoreID();

	for (unsigned int i = 0; i < index; i++)
	{
		if (task_vector[i].getCoreID() != coreID)
			continue;

		if (task_vector[i].getTotalGe() != 0)
			interference = interference + ceil((resp_time + resp_time_hp[i] - ((task_vector[i].getC()+task_vector[i].getTotalGm())))/task_vector[i].getT())*(task_vector[i].getC()+task_vector[i].getTotalGm());
		else
			interference = interference + ceil((resp_time)/task_vector[i].getT())*(task_vector[i].getC());
	}
	return interference;
}

/**************** The Calculate High-Priority response time sub-routine using the job-driven approach ********************/ 
std::vector<double> calculate_hp_resp_time_jd(unsigned int index, const std::vector<Task> &task_vector)
{
	double blocking, interference;
	double resp_time, resp_time_dash, init_resp_time;
	double deadline;
	std::vector<double> resp_time_hp(index, 0);

	for (unsigned int i = 0; i < index; i++)
	{
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		// -> here we use Hi instead of Gi to get the indirect and cis blocking taken care of
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalH();
		resp_time = init_resp_time;
		resp_time_dash = 0;
		deadline = task_vector[i].getD();
		while (resp_time != resp_time_dash && resp_time <= 5*deadline)
		{
			resp_time = resp_time_dash;
			// Get the blocking
			blocking = calculate_blocking_jd(i, task_vector, resp_time_hp, resp_time);
			interference = calculate_interference_jd(i, task_vector, resp_time_hp, resp_time);
			resp_time_dash = init_resp_time + blocking + interference;
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using the Job-Driven Approach ********************/ 
int check_schedulability_job_driven(std::vector<Task> &task_vector, std::vector<double> &resp_time)
{
	// Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	if (DEBUG)
		printf("Job-Driven Approach\n");

	// Do the schedulability test
	resp_time = calculate_hp_resp_time_jd(task_vector.size(), task_vector);

	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		if (resp_time[index] <= task_vector[index].getD())
		{
			if (DEBUG)
				printf("Task %d schedulable, response time = %f\n", index, resp_time[index]);
		}
		else
		{
			if (DEBUG)
				printf("Task %d not schedulable, response time = %f\n", index, resp_time[index]);
			return -1;
		}
	}
	return 0;
}

