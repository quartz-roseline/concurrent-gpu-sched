/*
 * @file job-driven-test-conc.cpp
 * @brief Implementing the job-driven schedulability test for concurrent GPUs
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
#include "config.hpp"

// Request-Oriented flag
bool request_oriented = false;

/**************** Calculate Prioritized Blocking using the job-driven approach sub-routine ********************/ 
double calculate_prioritized_blocking_jdc(unsigned int index, double response_time, const std::vector<Task> &task_vector)
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

/**************** Calculate Liquefaction Mass ********************/ 
double calculate_liquefaction_mass_jdc(unsigned int index, double resp_time, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp)
{
	double mass = 0;
	double alpha;
	unsigned int num_gpu_segments;
	double fraction = task_vector[index].getMaxF();
	double blocking_fraction = 1 - fraction + (1.0/double(GPU_FRACTION_GRANULARITY));
	double req_fraction;

	// Iterate over all tasks to calculate blocking
	for (unsigned int i = 0; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (task_vector[i].getTotalGe() != 0)
		{
			alpha = ceil((resp_time + resp_time_hp[i] - ((task_vector[i].getC()+task_vector[i].getTotalGm())))/task_vector[i].getT());
			for (unsigned int req_ind = 0; req_ind < num_gpu_segments; req_ind++)
			{
				if (task_vector[i].getGe(req_ind) != 0)
				{
					req_fraction = task_vector[i].getF(req_ind);
					// Consider all requests for high-prio tasks, and only smaller (fraction) requests for low-prio tasks
					if (i < index || req_fraction < fraction )
					{
						// Handle edge case optimization if the request fraction is bigger than the blocking fraction
						if (req_fraction <= blocking_fraction)
							mass = mass + alpha*(task_vector[i].getH(req_ind)*req_fraction);
						else
							mass = mass + alpha*(task_vector[i].getH(req_ind)*blocking_fraction);
					}
				}
			}
		}
	}
	return mass;
}

/**************** Calculate Per-job Request-Oriented Direct Blocking sub-routine ********************/ 
double calculate_direct_blocking_rojdc(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
{
	double blocking = 0;
	double blocking_dash;
	double lp_blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	double fraction = task_vector[index].getMaxF();
	double blocking_fraction;
	double left_over_fraction;
	double wavefront_req_fraction = 0;
	double liquefied_mass = 0 ;
	double wavefront_liquefaction_mass = 0;
	int num_biggest = 0;
	unsigned int max_index;

	double Hl_max = MAX_PERIOD+1; // Set to a large number (MAX_PERIOD is biggest possible)

	// Return 0 blocking if task has no GPU execution
	if(task_vector[index].getTotalGe() == 0)
		return 0;

	// Get the index of the biggest gpu fraction of this task
	task_vector[index].getIndexMaxF(0, max_index);

	for (unsigned int req_ind = 0; req_ind < num_gpu_segments; req_ind++)
	{
		// Get this task fraction
		fraction = task_vector[index].getF(req_ind);
		blocking_fraction = 1 - fraction + (1.0/double(GPU_FRACTION_GRANULARITY));
		left_over_fraction = blocking_fraction;

		Hl_max = MAX_PERIOD+1; // Set to a large number (MAX_PERIOD is biggest possible)
		num_biggest = 0;
		wavefront_liquefaction_mass = 0;
		wavefront_req_fraction = 0;

		// Get the wavefront pattern
		while (left_over_fraction > 0 && Hl_max > 0)
		{
			num_biggest++;
			Hl_max = find_next_max_lp_gpu_wcrt_segment_frac(index, Hl_max, num_biggest, wavefront_req_fraction, 
												  			fraction, task_vector);
			// Update the leftover fraction to fill
			left_over_fraction = left_over_fraction - wavefront_req_fraction;

			// add an optimization for the request bigger than the blocking fraction (if we use wavefront liquefaction)
			if (wavefront_req_fraction > blocking_fraction)
				wavefront_req_fraction = blocking_fraction;

			wavefront_liquefaction_mass = wavefront_liquefaction_mass + Hl_max*wavefront_req_fraction;
		}

		if (req_ind < max_index)
		{
			// Set the liquefaction mass as the low-prio blocking using liquefaction for this request
			liquefied_mass = wavefront_liquefaction_mass;
		}
		else
		{
			// Get the Liquefaction Mass
			liquefied_mass = wavefront_liquefaction_mass + calculate_liquefaction_mass_jdc(index, resp_time, 
														 	task_vector, resp_time_hp);

			// Get the next index of the biggest gpu fraction of this task
			task_vector[index].getIndexMaxF(req_ind+1, max_index);
		}
		
		// Compute the blocking estimate	
		blocking = blocking + floor(liquefied_mass/blocking_fraction);
	}
	
	return blocking;
}

/**************** Calculate Per-job Direct Blocking sub-routine ********************/ 
double calculate_direct_blocking_jdc(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
{
	double blocking = 0;
	double blocking_dash;
	double init_blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	double fraction = task_vector[index].getMaxF();
	double blocking_fraction = 1 - fraction + (1.0/double(GPU_FRACTION_GRANULARITY));
	double left_over_fraction = blocking_fraction;
	double wavefront_req_fraction = 0;
	double liquefied_mass = 0 ;
	double wavefront_liquefaction_mass = 0;
	int num_biggest = 0;

	double Hl_max = MAX_PERIOD+1; // Set to a large number (MAX_PERIOD is biggest possible)

	// Return 0 blocking if task has no GPU execution
	if(task_vector[index].getTotalGe() == 0)
		return 0;

	// Get the wavefront pattern
	while (left_over_fraction > 0 && Hl_max > 0)
	{
		num_biggest++;
		Hl_max = find_next_max_lp_gpu_wcrt_segment_frac(index, Hl_max, num_biggest, wavefront_req_fraction, 
											  			fraction, task_vector);
		// Update the leftover fraction to fill
		left_over_fraction = left_over_fraction - wavefront_req_fraction;

		// add an optimization for the request bigger than the blocking fraction (if we use wavefront liquefaction)
		if (wavefront_req_fraction > blocking_fraction)
			wavefront_req_fraction = blocking_fraction;

		wavefront_liquefaction_mass = wavefront_liquefaction_mass + Hl_max*wavefront_req_fraction;
	}

	// Get the Liquefaction Mass
	liquefied_mass = calculate_liquefaction_mass_jdc(index, resp_time, 
													 task_vector, resp_time_hp);
	// Add the wavefront mass	
	liquefied_mass = liquefied_mass + num_gpu_segments*wavefront_liquefaction_mass; 
	
	// Compute the blocking estimate	
	blocking = init_blocking + floor(liquefied_mass/blocking_fraction);
	
	return blocking;
}

/**************** Calculate Total Blocking sub-routine ********************/ 
double calculate_blocking_jdc(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time, std::vector<double> &direct_blocking)
{
	double blocking = 0;
	double direct_blocking_local = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();

	// Add the prioritized blocking using the job-driven approach
	blocking = blocking + calculate_prioritized_blocking_jdc(index, resp_time, task_vector);
	
	if (num_gpu_segments == 0)
		return blocking;

	// Get the direct blocking
	if (request_oriented) // Use the Request-Oriented Job-Driven approach
		direct_blocking_local = calculate_direct_blocking_rojdc(index, task_vector, resp_time_hp, resp_time);
	else // Use the regular job-oriented approach
		direct_blocking_local = calculate_direct_blocking_jdc(index, task_vector, resp_time_hp, resp_time);
	
	direct_blocking[index] = direct_blocking_local;
	blocking = blocking + direct_blocking_local;

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_jdc(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
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
std::vector<double> calculate_hp_resp_time_jdc(unsigned int index, const std::vector<Task> &task_vector, std::vector<double> &direct_blocking)
{
	double blocking, interference;
	double resp_time, resp_time_dash, init_resp_time;
	std::vector<double> resp_time_hp(index, 0);

	for (unsigned int i = 0; i < index; i++)
	{
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		// -> here we use Hi instead of Gi to get the indirect and cis blocking taken care of
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalH();
		resp_time = init_resp_time;
		resp_time_dash = 0;
		while (resp_time != resp_time_dash)
		{
			resp_time = resp_time_dash;
			// Get the blocking
			blocking = calculate_blocking_jdc(i, task_vector, resp_time_hp, resp_time, direct_blocking);
			interference = calculate_interference_jdc(i, task_vector, resp_time_hp, resp_time);
			resp_time_dash = init_resp_time + blocking + interference;
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using the Job-Driven Approach ********************/ 
int check_schedulability_job_driven_conc(std::vector<Task> &task_vector, std::vector<double> &resp_time,
										 std::vector<double> &direct_blocking, bool ro_job_flag)
{
	// Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	// Set the direct blocking vector
	direct_blocking.clear();
	direct_blocking.reserve(task_vector.size());

	// Set the request-oriented flag
	request_oriented = ro_job_flag;

	// Do the schedulability test
	resp_time = calculate_hp_resp_time_jdc(task_vector.size(), task_vector, direct_blocking);

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

