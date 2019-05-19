/*
 * @file request-driven-test.cpp
 * @brief Implementing the request-driven schedulability test (for non-concurrent GPUs)
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
/* Standard Library Imports */
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

/* Internal Headers */
#include "request-driven-test.hpp"
#include "indirect-cis.hpp"
#include "taskset.hpp"

/**************** Calculate Prioritized Blocking using the request-driven approach sub-routine ********************/ 
double calculate_prioritized_blocking_rd(unsigned int index, const std::vector<Task> &task_vector)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	unsigned int coreID = task_vector[index].getCoreID();

	// Calculate the prioritized blocking due to low-prio tasks on the same core
	for (unsigned int i = index + 1; i < task_vector.size(); i++)
	{
		// Tasks not on our core do not contribute
		if (task_vector[i].getCoreID() != coreID)
			continue;

		blocking = blocking + task_vector[i].getMaxGm();
	}

	blocking = (num_gpu_segments + 1)*blocking;
	return blocking;
}

/**************** Calculate Per-Request Direct Blocking sub-routine ********************/ 
double calculate_request_direct_blocking_rd(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp)
{
	double blocking, blocking_dash, beta;
	unsigned int num_gpu_segments;

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
		blocking = Hl_max;
		// Ensure the initial value of blocking_dash is different from blocking
		if (Hl_max == 0)
		{
			blocking_dash = 1;
		}
		else
		{
			blocking_dash = 0;
		}
	}
	else
	{
		// No other lower priority tasks exist -> Vector Index has overrun
		blocking = 0;
		
		// Ensure the initial value of blocking_dash is different from blocking
		blocking_dash = 1;
	}
	
	// Estimate Blocking
	while (blocking != blocking_dash)
	{
		blocking_dash = blocking;
		blocking = Hl_max;
		for (unsigned int i = 0; i < index; i++)
		{
			num_gpu_segments = task_vector[i].getNumGPUSegments();
			if (task_vector[i].getTotalGe() != 0)
			{
				for (unsigned int req_ind = 0; req_ind < num_gpu_segments; req_ind++)
				{
					if (task_vector[i].getGe(req_ind) != 0)
					{
						beta = ceil((blocking_dash + resp_time_hp[i] - ((task_vector[i].getC()+task_vector[i].getTotalGm())))/task_vector[i].getT());
						blocking = blocking + beta*(task_vector[i].getH(req_ind));
					}
				}
			}
		}
	}
	return blocking;
}

/**************** Calculate Per-Request Blocking sub-routine ********************/ 
double calculate_request_blocking_rd(unsigned int index, unsigned int req_index, 
									 const std::vector<Task> &task_vector, 
									 const std::vector<double> &resp_time_hp,
									 std::vector<std::vector<double>> &req_blocking)
{
	double direct_blocking, blocking = 0;
	double G = task_vector[index].getG(req_index);

	// Return 0 if GPU segment is zero
	if (G == 0)
		return 0;

	// Compute the combined direct blocking, and the indirect and cis faced by the request
	direct_blocking = calculate_request_direct_blocking_rd(index, req_index, task_vector, resp_time_hp);
	req_blocking[index].push_back(direct_blocking);
	blocking = direct_blocking
	+ calculate_request_indirect_blocking(index, req_index, task_vector)
	+ calculate_request_cis(index, req_index, task_vector);

	return blocking;
}

/**************** Calculate Total Blocking sub-routine ********************/ 
double calculate_blocking_rd(unsigned int index, const std::vector<Task> &task_vector, 
							 const std::vector<double> &resp_time_hp,
							 std::vector<std::vector<double>> &req_blocking)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	std::vector<double> empty_vector;

	// Push back an empty vector
	req_blocking.push_back(empty_vector);

	// Add the prioritized blocking using the request-driven approach -> faced even by tasks with no gpu requests
	blocking = blocking + calculate_prioritized_blocking_rd(index, task_vector);

	if (num_gpu_segments == 0)
		return blocking;
	
	// Get the per-request blocking (direct, indirect and concurrency-induced serialization)
	for (unsigned int req_index = 0; req_index < num_gpu_segments; req_index++)
	{
		blocking = blocking + calculate_request_blocking_rd(index, req_index, task_vector, resp_time_hp, req_blocking);
	}

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_rd(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
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

/**************** The Calculate High-Priority response time sub-routine using the request-driven approach ********************/ 
std::vector<double> calculate_hp_resp_time_rd(unsigned int index, const std::vector<Task> &task_vector, 
											  std::vector<std::vector<double>> &req_blocking)
{
	double blocking;
	double resp_time, resp_time_dash, init_resp_time;
	std::vector<double> resp_time_hp(index, 0);

	for (unsigned int i = 0; i < index; i++)
	{
		// Get the blocking
		blocking = calculate_blocking_rd(i, task_vector, resp_time_hp, req_blocking);
		
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalG() + blocking;
		resp_time = init_resp_time;
		resp_time_dash = 0;
		while (resp_time != resp_time_dash)
		{
			resp_time = resp_time_dash;
			resp_time_dash = init_resp_time + calculate_interference_rd(i, task_vector, resp_time_hp, resp_time);
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using the Request-Driven Approach ********************/ 
int check_schedulability_request_driven(std::vector<Task> &task_vector, 
										std::vector<double> &resp_time, 
										std::vector<std::vector<double>> &req_blocking)
{
	// Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	// Clear the direct blocking vector of vectors
	req_blocking.clear();

	// Do the schedulability test
	resp_time = calculate_hp_resp_time_rd(task_vector.size(), task_vector, req_blocking);

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

