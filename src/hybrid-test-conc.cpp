/*
 * @file hybrid-test.cpp
 * @brief Implementing the hybrid schedulability test for concurrent GPUs
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
#include "hybrid-test.hpp"
#include "indirect-cis.hpp"
#include "taskset.hpp"
#include "config.hpp"

/**************** Calculate Prioritized Blocking using the hybrid approach sub-routine ********************/ 
double calculate_prioritized_blocking_hybrid_conc(unsigned int index, double response_time, const std::vector<Task> &task_vector)
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

/**************** Calculate Extra Differential Direct blocking and prioritized blocking ********************/ 
double calculate_blocking_hybrid_diff_conc(unsigned int index, const std::vector<Task> &task_vector, 
								 const std::vector<double> &resp_time_hp, double resp_time)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();

	// Add the prioritized blocking using the hybrid approach -> faced by all tasks, even without gpu segments
	blocking = blocking + calculate_prioritized_blocking_hybrid_conc(index, resp_time, task_vector);

	return blocking;
}

/**************** Calculate Direct blocking due to high-prio tasks only ********************/ 
double calculate_blocking_hybrid_direct_init_conc(unsigned int index, const std::vector<Task> &task_vector, 
								 const std::vector<double> &resp_time_rd,		
								 const std::vector<double> &resp_time_jd,
								 const std::vector<std::vector<double>> &req_blocking,
								 const std::vector<double> &job_blocking)
{
	double blocking = 0;
	double rd_blocking = 0;
	double jd_blocking = 0;
	unsigned int num_gpu_segments_blk = task_vector[index].getNumGPUSegments(); // num gpu requests of blocked task

	if (num_gpu_segments_blk == 0)
		return blocking;

	// Compute the blocking due to the request-driven approach -> add individual request blockings
	for (unsigned int req_index = 0; req_index < num_gpu_segments_blk; req_index++)
	{ 
		rd_blocking = rd_blocking + req_blocking[index][req_index];
	}

	// Get the blocking due to the job-driven approach
	jd_blocking = job_blocking[index];

	// Chose the minimum as the final direct blocking as both are valid bounds
	if (jd_blocking < rd_blocking)
		blocking = jd_blocking;
	else
		blocking = rd_blocking;

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_hybrid_conc(unsigned int index, const std::vector<Task> &task_vector, 
									 const std::vector<double> &resp_time_hp, double resp_time)
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

/**************** The Calculate High-Priority response time sub-routine using the hybrid approach ********************/ 
std::vector<double> calculate_hp_resp_time_hybrid_conc(unsigned int index, const std::vector<Task> &task_vector, 
													const std::vector<double> &resp_time_rd,
													const std::vector<double> &resp_time_jd,
													const std::vector<std::vector<double>> &req_blocking,
													const std::vector<double> &job_blocking)
{
	double blocking, blocking_init, interference;
	double resp_time, resp_time_dash, init_resp_time;
	double deadline;
	std::vector<double> resp_time_hp(index, 0);

	// Set the response time to the deadline initially (as we have to use low-prio in our blocking calc)
	for (unsigned int i = 0; i < index; i++)
	{
		resp_time_hp[i] = task_vector[i].getD();
	}

	for (unsigned int i = 0; i < index; i++)
	{
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		// -> here we use Hi instead of Gi to get the indirect and cis blocking taken care of
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalH();
		blocking_init = calculate_blocking_hybrid_direct_init_conc(i, task_vector, resp_time_rd, resp_time_jd, req_blocking, job_blocking);
		resp_time = init_resp_time;
		resp_time_dash = 0;
		deadline = task_vector[i].getD();
		while (resp_time != resp_time_dash && resp_time <= deadline)
		{
			resp_time = resp_time_dash;
			// Get the blocking
			blocking = blocking_init + calculate_blocking_hybrid_diff_conc(i, task_vector, resp_time_hp, resp_time);
			interference = calculate_interference_hybrid_conc(i, task_vector, resp_time_hp, resp_time);
			resp_time_dash = init_resp_time + blocking + interference;
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using the Hybrid Approach ********************/ 
int check_schedulability_hybrid_conc(std::vector<Task> &task_vector,
								const std::vector<double> &resp_time_rd,
								const std::vector<double> &resp_time_jd,
								const std::vector<std::vector<double>> &req_blocking,
								const std::vector<double> &job_blocking)
{
	//l Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	if (DEBUG)
		printf("Concurrent Hybrid Approach\n");

	// Do the schedulability test
	std::vector<double> resp_time = calculate_hp_resp_time_hybrid_conc(task_vector.size(), task_vector,
																  resp_time_rd, resp_time_jd, req_blocking, job_blocking);

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

