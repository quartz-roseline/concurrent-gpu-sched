/*
 * @file request-driven-test-conc.cpp
 * @brief Implementing the request-driven schedulability test for concurrent GPUs
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
#include "request-driven-test-conc.hpp"
#include "indirect-cis.hpp"
#include "taskset.hpp"
#include "config.hpp"

// Wavefront liquefaction flag
bool wavefront_liquefaction = false;

/**************** Calculate Prioritized Blocking using the request-driven approach sub-routine ********************/ 
double calculate_prioritized_blocking_rdc(unsigned int index, const std::vector<Task> &task_vector)
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

/**************** Calculate Liquefaction Mass ********************/ 
double calculate_liquefaction_mass_rdc(unsigned int index, unsigned int req_index, double instant, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp)
{
	double mass = 0;
	double beta;
	unsigned int num_gpu_segments;
	double fraction = task_vector[index].getF(req_index);
	double blocking_fraction = 1 - fraction + (1.0/double(GPU_FRACTION_GRANULARITY));
	double req_fraction;

	// Iterate over all tasks to calculate blocking
	for (unsigned int i = 0; i < task_vector.size(); i++)
	{
		num_gpu_segments = task_vector[i].getNumGPUSegments();
		if (task_vector[i].getTotalGe() != 0 && i != index)
		{
			beta = ceil((instant + resp_time_hp[i] - ((task_vector[i].getC()+task_vector[i].getTotalGm())))/task_vector[i].getT());
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
							mass = mass + beta*(task_vector[i].getH(req_ind)*req_fraction);
						else
							mass = mass + beta*(task_vector[i].getH(req_ind)*blocking_fraction);
					}
				}
			}
		}
	}
	return mass;
}

/**************** Calculate Per-Request Direct Blocking sub-routine ********************/ 
double calculate_request_direct_blocking_rdc(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp)
{
	double blocking = 0;
	double blocking_dash;
	double init_blocking = 0;
	unsigned int num_gpu_segments;
	double fraction = task_vector[index].getF(req_index);
	double blocking_fraction = 1 - fraction + (1.0/double(GPU_FRACTION_GRANULARITY));
	double left_over_fraction = blocking_fraction;
	double wavefront_req_fraction = 0;
	double liquefied_mass = 0 ;
	double liquefied_mass_used = 0;
	double required_mass = 0;
	double wavefront_liquefaction_mass = 0;
	int num_biggest = 0;
	unsigned int instant = 0;
	unsigned int prev_instant = 0;
	unsigned int num_bins, prev_num_bins;
	std::vector<double> wavefront_length;
	std::vector<double> wavefront_bin_fraction;

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
		wavefront_length.push_back(Hl_max);
		wavefront_bin_fraction.push_back(left_over_fraction);

		// Add an optimization for the request bigger than the blocking fraction (if we use wavefront liquefaction)
		// if (wavefront_req_fraction > blocking_fraction)
		// 	wavefront_req_fraction = blocking_fraction;

		if (left_over_fraction < 0)
			wavefront_req_fraction = wavefront_req_fraction + left_over_fraction;

		wavefront_liquefaction_mass = wavefront_liquefaction_mass + Hl_max*wavefront_req_fraction;
	}

	// Do liquefaction for the first k partially-filled bins occupied by the wavefront
	if (!wavefront_liquefaction)
	{
		for (int i = num_biggest - 1; i >= 0; i--)
		{
			prev_instant = instant;
			// Bin already full, discard
			if (wavefront_bin_fraction[i] <= 0)
			{
				// Get the next instant at which we calculate the liquefied mass
				instant = (unsigned int)wavefront_length[i];
				continue;
			}

			// Get liquefied mass
			liquefied_mass = calculate_liquefaction_mass_rdc(index, req_index, (double)instant, task_vector, resp_time_hp) 
							 - liquefied_mass_used;

			// Get the next instant at which we calculate the liquefied mass
			instant = (unsigned int)wavefront_length[i];

			// Get the required mass of the set of bins
			required_mass = (instant - prev_instant)*wavefront_bin_fraction[i];

			num_bins = 0;
			prev_num_bins = 0;
			// If liquefied mass is less than required mass
			while (liquefied_mass < required_mass)
			{
				num_bins = (unsigned int)floor((liquefied_mass/required_mass)*(instant - prev_instant));
				// Check if the recurrence has converged -> return the blocking
				if (prev_num_bins == num_bins)
				{
					blocking = prev_instant + num_bins;
					return blocking;
				}

				// Get new mass at new instant
				liquefied_mass = calculate_liquefaction_mass_rdc(index, req_index, (double) prev_instant+num_bins, task_vector, resp_time_hp)
								 - liquefied_mass_used;
				prev_num_bins = num_bins;
			}

			// Account for used mass
			liquefied_mass_used = liquefied_mass_used + required_mass;

			// Update the blocking
			blocking = instant;
		}
	}

	// Estimate Blocking -> First initialize
	init_blocking = blocking;
	blocking_dash = blocking + 1;
	// Perform recurrence until convergence
	while (blocking != blocking_dash)
	{
		blocking_dash = blocking;
		liquefied_mass = calculate_liquefaction_mass_rdc(index, req_index, blocking, 
														 task_vector, resp_time_hp);
						    
		// Is the wavefront liquefaction flag set
		if (wavefront_liquefaction)
		{
			liquefied_mass = liquefied_mass + wavefront_liquefaction_mass; // add the wavefront mass
		}
		else 
		{
			 liquefied_mass = liquefied_mass - liquefied_mass_used; // subtract the mass used to fill the bins at the wavefront
		}
		blocking = init_blocking + floor(liquefied_mass/blocking_fraction);
				
	}
	return blocking;
}

/**************** Calculate Per-Request Blocking sub-routine ********************/ 
double calculate_request_blocking_rdc(unsigned int index, unsigned int req_index, 
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
	direct_blocking = calculate_request_direct_blocking_rdc(index, req_index, task_vector, resp_time_hp);
	req_blocking[index].push_back(direct_blocking);
	blocking = direct_blocking
	+ calculate_request_indirect_blocking(index, req_index, task_vector)
	+ calculate_request_cis(index, req_index, task_vector);

	return blocking;
}

/**************** Calculate Total Blocking sub-routine ********************/ 
double calculate_blocking_rdc(unsigned int index, const std::vector<Task> &task_vector, 
							 const std::vector<double> &resp_time_hp,
							 std::vector<std::vector<double>> &req_blocking)
{
	double blocking = 0;
	unsigned int num_gpu_segments = task_vector[index].getNumGPUSegments();
	std::vector<double> empty_vector;

	// Push back an empty vector
	req_blocking.push_back(empty_vector);

	// Add the prioritized blocking using the request-driven approach -> even faced by tasks without gpu segments
	blocking = blocking + calculate_prioritized_blocking_rdc(index, task_vector);

	if (num_gpu_segments == 0)
		return blocking;
	
	// Get the per-request blocking (direct, indirect and concurrency-induced serialization)
	for (unsigned int req_index = 0; req_index < num_gpu_segments; req_index++)
	{
		blocking = blocking + calculate_request_blocking_rdc(index, req_index, task_vector, resp_time_hp, req_blocking);
	}

	return blocking;
}

/**************** Calculate High-Priority Interference sub-routine ********************/ 
double calculate_interference_rdc(unsigned int index, const std::vector<Task> &task_vector, const std::vector<double> &resp_time_hp, double resp_time)
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

/**************** The Calculate High-Priority response time using the concurrent request-driven approach ********************/ 
std::vector<double> calculate_hp_resp_time_rdc(unsigned int index, const std::vector<Task> &task_vector, 
											  std::vector<std::vector<double>> &req_blocking)
{
	double blocking;
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
		// Get the blocking
		blocking = calculate_blocking_rdc(i, task_vector, resp_time_hp, req_blocking);
		deadline = task_vector[i].getD();
		// Calculate the blocking using the recurrence Wi = Ci + Gi + Bi + Interference
		init_resp_time = task_vector[i].getC() + task_vector[i].getTotalG() + blocking;
		resp_time = init_resp_time;
		resp_time_dash = 0;
		while (resp_time != resp_time_dash && resp_time <= 5*deadline)
		{
			resp_time = resp_time_dash;
			resp_time_dash = init_resp_time + calculate_interference_rdc(i, task_vector, resp_time_hp, resp_time);
		}
		resp_time_hp[i] = resp_time;
	}

	return resp_time_hp;
}

/**************** Calculate Schedulability using the Request-Driven Approach ********************/ 
int check_schedulability_request_driven_conc(std::vector<Task> &task_vector, 
										std::vector<double> &resp_time, 
										std::vector<std::vector<double>> &req_blocking, 
										bool simple_flag)
{
	// Pre-compute the response-time of each GPU segment
	pre_compute_gpu_response_time(task_vector);

	// Set the global wavefront liquefaction flag
	wavefront_liquefaction = simple_flag;

	if (DEBUG)
		printf("Concurrent Request-Driven Approach %d\n", simple_flag);

	// Clear the direct blocking vector of vectors
	req_blocking.clear();

	// Do the schedulability test
	resp_time = calculate_hp_resp_time_rdc(task_vector.size(), task_vector, req_blocking);

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

