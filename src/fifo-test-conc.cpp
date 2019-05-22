/*
 * @file hybrid-test-conc.cpp
 * @brief Implementing the fifo test for concurrent GPUs
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
#include "fifo-test-conc.hpp"
#include "indirect-cis.hpp"
#include "taskset.hpp"
#include "config.hpp"

/**************** Calculate Prioritized Blocking using the hybrid approach (same for fifo) sub-routine ********************/ 
double calculate_prioritized_blocking_fifo_hybrid_conc(unsigned int index, double response_time, const std::vector<Task> &task_vector)
{
	double blocking = 0;
	unsigned int theta = 0;
	int num_biggest = 1;
	double phi, phi_sum, Gm_max;
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

		// Calculate the prioritized blocking due to this lp task
		phi = 0;
		phi_sum = 0;
		Gm_max = find_next_task_max_gpu_intervention_segment(i, double(MAX_PERIOD+1), num_biggest, task_vector);
		while (num_gpu_segments + 1 - phi_sum > 0 && Gm_max > 0)
		{
			if (theta < num_gpu_segments + 1 - phi_sum)
				phi = theta;
			else
				phi = num_gpu_segments + 1 - phi_sum;

			// Add to the phi sum
			phi_sum = phi_sum + phi;

			// Add to the blocking
			blocking = blocking + phi*Gm_max;

			// Get next biggest request
			num_biggest++;
			Gm_max = find_next_task_max_gpu_intervention_segment(i, Gm_max, num_biggest, task_vector);
		}

	}

	return blocking;
}

/**************** Calculate Per-Request Direct Blocking sub-routine ********************/ 
double calculate_request_direct_blocking_fifo(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp)
{
	double blocking = 0;

	// Return 0 blocking if task has no GPU execution
	if(task_vector[index].getTotalGe() == 0)
		return 0;
	
	// Estimate Blocking -> take the biggest wcrt gpu segment from each task
	for (unsigned int i = 0; i < task_vector.size(); i++)
	{
		// Cannot be blocked by myself
		if (i == index)
			continue;

		if (task_vector[i].getTotalGe() != 0)
		{	
			blocking = blocking + task_vector[i].getMaxH();	
		}
	}
	return blocking;
}

/**************** Calculate Per-Request Blocking sub-routine ********************/ 
double calculate_request_blocking_fifo(unsigned int index, unsigned int req_index, 
									 const std::vector<Task> &task_vector, 
									 const std::vector<double> &resp_time_hp)
{
	double direct_blocking, blocking = 0;
	double G = task_vector[index].getG(req_index);

	// Return 0 if GPU segment is zero
	if (G == 0)
		return 0;

	// Compute the combined direct blocking, and the indirect and cis faced by the request
	direct_blocking = calculate_request_direct_blocking_fifo(index, req_index, task_vector, resp_time_hp);
	
	blocking = direct_blocking
	+ calculate_request_indirect_blocking(index, req_index, task_vector)
	+ calculate_request_cis(index, req_index, task_vector);

	return blocking;
}

/**************** Calculate Total Blocking sub-routine ********************/ 
double calculate_blocking_fifo(unsigned int index, const std::vector<Task> &task_vector, 
							 const std::vector<double> &resp_time_hp)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	std::vector<double> empty_vector;

	if (num_gpu_segments == 0)
		return blocking;
	
	// Get the per-request blocking (direct, indirect and concurrency-induced serialization)
	for (unsigned int req_index = 0; req_index < num_gpu_segments; req_index++)
	{
		blocking = blocking + calculate_request_blocking_fifo(index, req_index, task_vector, resp_time_hp);
	}

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_fifo(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
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
std::vector<double> calculate_hp_resp_time_fifo(unsigned int index, const std::vector<Task> &task_vector)
{
	double blocking, prioritized_blocking;
	double resp_time, resp_time_dash, init_resp_time;
	double deadline;
	std::vector<double> resp_time_hp(index, 0);

	for (unsigned int i = 0; i < index; i++)
	{
		// Get the blocking
		blocking = calculate_blocking_fifo(i, task_vector, resp_time_hp);
		
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalG() + blocking;
		resp_time = init_resp_time;
		resp_time_dash = 0;
		deadline = task_vector[i].getD();
		while (resp_time != resp_time_dash && resp_time <= 5*deadline)
		{
			resp_time = resp_time_dash;
			// Add the prioritized blocking using the hybrid approach -> faced even by tasks with no gpu requests
			prioritized_blocking =  calculate_prioritized_blocking_fifo_hybrid_conc(i, resp_time, task_vector);
			resp_time_dash = init_resp_time + prioritized_blocking 
							+ calculate_interference_fifo(i, task_vector, resp_time_hp, resp_time);
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using FIFO on the concurrent GPU ********************/ 
int check_schedulability_fifo_conc(std::vector<Task> &task_vector)
{
	// Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	if (DEBUG)
		printf("FIFO Policy on the GPU Approach\n");

	// Do the schedulability test
	std::vector<double> resp_time = calculate_hp_resp_time_fifo(task_vector.size(), task_vector);

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

