/*
 * @file indirect-cis.cpp
 * @brief Implementing the indirect blocking and concurrency-induced serialization routines
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
#include "indirect-cis.hpp"
#include "taskset.hpp"

/**************** Calculate Per-Request Concurrency-Induced Serialization sub-routine ********************/ 
double calculate_request_cis(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector)
{
	double blocking = 0;
	int eta = 1; // The number of suspensions per-critical section, hardcoded to 1
	unsigned int coreID = task_vector[index].getCoreID();
	double request_frac_req = task_vector[index].getF(req_index);
	double G = task_vector[index].getG(req_index);

	// Return 0 if GPU segment is zero
	if (G == 0)
		return 0;

	// Calculate the concurrency-induced serialization due to each high-priority task
	for (unsigned int i = 0; i < index; i++)
	{
		// Tasks not on our core do not contribute
		if (task_vector[i].getCoreID() != coreID)
			continue; 

		// Get maximum task CPU intervention, with fraction on GPU <= 1 - the fraction required by this request
		blocking = blocking + task_vector[i].getMaxGmLeqFraction(1 - request_frac_req);
	}
	return (eta + 1)*blocking;
}

/**************** Calculate Per-Request Indirect Blocking sub-routine ********************/ 
double calculate_request_indirect_blocking(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector)
{
	// For now we assume only a single GPU, so there is no indirect blocking due to other resources
	return 0;
}

/**************** Calculate the worst-case response time of a request sub-routine ********************/ 
double calculate_request_response_time(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector)
{
	double H = 0;

	if (task_vector[index].getG(req_index)  == 0)
		return 0;

	// Worst-case response time = WCET + Indirect Blocking + Concurrency-induced serialization
	H = task_vector[index].getG(req_index) 
		+ calculate_request_indirect_blocking(index, req_index, task_vector)
		+ calculate_request_cis(index, req_index, task_vector);

	return H;
}

/**************** Calculate the worst-case response time of all gpu requests sub-routine ********************/ 
int pre_compute_gpu_response_time(std::vector<Task> &task_vector)
{
	unsigned int num_gpu_segments;
	// Set the response time of each gpu request of each task
	for (unsigned int index = 0; index < task_vector.size(); index++) 
	{
		num_gpu_segments = task_vector[index].getNumGPUSegments();
		for (unsigned int req_index = 0; req_index < num_gpu_segments; req_index++)
			task_vector[index].setH(req_index, calculate_request_response_time(index, req_index, task_vector));
	}
	return 0;
}