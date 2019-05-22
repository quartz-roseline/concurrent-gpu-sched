/*
 * @file task_partitioning.cpp
 * @brief WFD-based Task Partitioning
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
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

/* Internal Headers */
#include "task_partitioning.hpp"
#include "taskset.hpp"

/* Schedulability test headers */
#include "request-driven-test.hpp"
#include "job-driven-test.hpp"
#include "hybrid-test.hpp"
#include "request-driven-test-conc.hpp"
#include "job-driven-test-conc.hpp"
#include "hybrid-test-conc.hpp"
#include "fifo-test-conc.hpp"

// Comparator class for ordering tasks in decreasing order of utilization
struct CompareTaskUtil {
    bool operator()(Task const & t1, Task const & t2) {
        // return "true" if "t1" has higher RMS priority than "t2"
        return ((t1.getC()+t1.getTotalGm())/t1.getT()) > ((t2.getC()+t2.getTotalGm())/t2.getT());
    }
} CompareTaskUtilWFD;

/**************** Find the core with the minimum utilization ********************/ 
unsigned int find_minutil_core(std::vector<double> &core_util, unsigned int start_core)
{
	unsigned int min_index = 0;
	double min_util = 2.0;

	for (unsigned int index = start_core; index < core_util.size(); index++)
	{
		if(core_util[index] < min_util)
		{
			min_util = core_util[index];
			min_index = index;
		}
	}
	return min_index;
}

/**************** Find the core with the minimum utilization excluding certain cores ********************/ 
unsigned int find_minutil_core_excluding(std::vector<double> &core_util, unsigned int start_core, std::vector<unsigned int> &exclusion_list)
{
	unsigned int min_index = 0;
	double min_util = 2.0;

	for (unsigned int index = start_core; index < core_util.size(); index++)
	{
		// Continue if the  index is in the exclusion list
		if(std::find(exclusion_list.begin(), exclusion_list.end(), index) != exclusion_list.end())
			continue;

		if(core_util[index] < min_util)
		{
			min_util = core_util[index];
			min_index = index;
		}
	}
	return min_index;
}

/**************** Utility function to check schedulability ********************/ 
static int check_schedulability(std::vector<Task> &task_vector, sched_type sched_mode,
			 			std::vector<double> &resp_time_rd,
						std::vector<double> &resp_time_jd,
						std::vector<std::vector<double>> &req_blocking_rd,
						std::vector<double> &job_blocking_jd)
{
	int sched_flag = -1;
	// Check Schedulability
	switch (sched_mode)
	{
		// Check Schedulability -> Non-concurrent approaches
		case REQUEST_DRIVEN:
			sched_flag = check_schedulability_request_driven(task_vector, resp_time_rd, req_blocking_rd);
			break;

		case JOB_DRIVEN:
			sched_flag = check_schedulability_job_driven(task_vector, resp_time_jd);
			break;
			
		case HYBRID:
			resp_time_rd.clear();
			req_blocking_rd.clear();
			resp_time_jd.clear();
			check_schedulability_request_driven(task_vector, resp_time_rd, req_blocking_rd);
			check_schedulability_job_driven(task_vector, resp_time_jd);
			sched_flag = check_schedulability_hybrid(task_vector, resp_time_rd, resp_time_jd, req_blocking_rd);
			break;
			
		// Check Schedulability -> Concurrent approaches (simple)
		case REQUEST_DRIVEN_CONC_SIMPLE:	
			sched_flag = check_schedulability_request_driven_conc(task_vector, resp_time_rd, req_blocking_rd, true);
			break;

		case JOB_DRIVEN_CONC:
			sched_flag = check_schedulability_job_driven_conc(task_vector, resp_time_jd, job_blocking_jd, false);
			break;
			
		// Check Schedulability -> Concurrent approaches (complex)
		case REQUEST_DRIVEN_CONC: 	
			sched_flag = check_schedulability_request_driven_conc(task_vector, resp_time_rd, req_blocking_rd, false);
			break;

		case JOB_DRIVEN_CONC_RO:
			sched_flag = check_schedulability_job_driven_conc(task_vector, resp_time_jd, job_blocking_jd, true);
			break;

		case HYBRID_CONC:
			resp_time_rd.clear();
			req_blocking_rd.clear();
			resp_time_jd.clear();
			job_blocking_jd.clear();
			check_schedulability_request_driven_conc(task_vector, resp_time_rd, req_blocking_rd, false);
			check_schedulability_job_driven_conc(task_vector, resp_time_jd, job_blocking_jd, true);
			sched_flag = check_schedulability_hybrid_conc(task_vector, resp_time_rd, resp_time_jd, req_blocking_rd, job_blocking_jd);
			break;

		case FIFO_CONC:
			sched_flag = check_schedulability_fifo_conc(task_vector);
			break;

		default:
			return -1;
	}
	return sched_flag;
}

/**************** The WFD Partitioning Algorithm ********************/ 
int worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, sched_type sched_mode,
						 std::vector<double> &resp_time_rd,
						 std::vector<double> &resp_time_jd,
						 std::vector<std::vector<double>> &req_blocking_rd,
						 std::vector<double> &job_blocking_jd,
						 std::function<bool(Task const &, Task const &)> priority_ordering)
{
	std::vector<Task> wfd_ordered_tasks;
	std::vector<Task> wfd_mapped_tasks;
	std::vector<double> core_util(num_cores, 0.0);
	std::vector<unsigned int> exclusion_list;
	unsigned int chosen_core;
	int sched_flag;

	// Mark all cores as un-allocated -> setCoreId = num_cores
	for (unsigned int index = 0; index < task_vector.size(); index++)
		task_vector[index].setCoreID(num_cores);

	// Initialize WFD vector
	wfd_ordered_tasks = task_vector;

	// Sort Vector based on Utilization
	std::sort(wfd_ordered_tasks.begin(), wfd_ordered_tasks.end(), CompareTaskUtilWFD);

	for (unsigned int index = 0; index < wfd_ordered_tasks.size(); index++)
	{
		sched_flag = 1;
		exclusion_list.clear();
		double task_util = (wfd_ordered_tasks[index].getC()+wfd_ordered_tasks[index].getTotalGm())/wfd_ordered_tasks[index].getT();

		// Try cores until schedulable
		while (sched_flag != 0)
		{
			// Find core with min util
			chosen_core = find_minutil_core_excluding(core_util, 0, exclusion_list);

			// Allocate the task to that core
			wfd_ordered_tasks[index].setCoreID(chosen_core);

			// Add the task to the vector of tasks
			if (sched_flag == 1)
				wfd_mapped_tasks.push_back(wfd_ordered_tasks[index]);

			// Sort Tasks according to RMS
			std::sort(wfd_mapped_tasks.begin(), wfd_mapped_tasks.end(), priority_ordering);

			// Check Schedulability
			sched_flag = check_schedulability(wfd_mapped_tasks, sched_mode, resp_time_rd, 
										      resp_time_jd, req_blocking_rd, job_blocking_jd);

			// If not schedulable declare an unfeasible partition
			if (sched_flag != 0)
			{
				if (exclusion_list.size() == num_cores-1)
				{
					return -1;
				}
				else
					exclusion_list.push_back(chosen_core);
			}
		}

		// Update the core utilization
		core_util[chosen_core] = core_util[chosen_core] + task_util;
	}

	// Copy over vector with core mappings 
	task_vector = wfd_mapped_tasks;
	return 0;
}

/**************** The Synchronization-Aware WFD Partitioning Algorithm ********************/ 
int sync_aware_worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, sched_type sched_mode,
						 			std::vector<double> &resp_time_rd,
									std::vector<double> &resp_time_jd,
									std::vector<std::vector<double>> &req_blocking_rd,
									std::vector<double> &job_blocking_jd,
									std::function<bool(Task const &, Task const &)> priority_ordering)
{
	std::vector<Task> wfd_ordered_tasks;
	std::vector<Task> wfd_mapped_tasks;
	std::vector<double> core_util(num_cores, 0.0);
	std::vector<unsigned int> exclusion_list;
	unsigned int chosen_core;
	int sched_flag;

	// CPU Utilization of tasks using the GPU 
	double cpu_gputil = get_gputasks_cpu_util(task_vector);

	// Taskset Utilization
	double cpu_util = get_taskset_cpu_util(task_vector);

	// Cores Reserved for Self suspending tasks
	int susp_cores = ceil(cpu_gputil/cpu_util)*num_cores;

	// Mark all cores as un-allocated -> setCoreId = num_cores
	for (unsigned int index = 0; index < task_vector.size(); index++)
		task_vector[index].setCoreID(num_cores);

	// Initialize WFD vector
	wfd_ordered_tasks = task_vector;

	// Sort Vector based on Utilization
	std::sort(wfd_ordered_tasks.begin(), wfd_ordered_tasks.end(), CompareTaskUtilWFD);

	// Assign tasks with self suspensions first
	for (unsigned int index = 0; index < wfd_ordered_tasks.size(); index++)
	{		
		// Find core with min util
		if (wfd_ordered_tasks[index].getTotalGe() == 0)
			continue;

		sched_flag = 1;
		exclusion_list.clear();
		double task_util = (wfd_ordered_tasks[index].getC()+wfd_ordered_tasks[index].getTotalGm())/wfd_ordered_tasks[index].getT();

		// Try cores until schedulable
		while (sched_flag != 0)
		{
			// Find core with min util
			chosen_core = find_minutil_core_excluding(core_util, num_cores - susp_cores, exclusion_list);

			// Allocate the task to that core
			wfd_ordered_tasks[index].setCoreID(chosen_core);

			// Add the task to the vector of tasks
			if (sched_flag == 1)
				wfd_mapped_tasks.push_back(wfd_ordered_tasks[index]);

			// Sort Tasks according to RMS
			std::sort(wfd_mapped_tasks.begin(), wfd_mapped_tasks.end(), priority_ordering);

			// Check Schedulability
			sched_flag = check_schedulability(wfd_mapped_tasks, sched_mode, resp_time_rd, 
										      resp_time_jd, req_blocking_rd, job_blocking_jd);

			// If not schedulable declare an unfeasible partition
			if (sched_flag != 0)
			{
				if (exclusion_list.size() == susp_cores)
					return -1;
				else
					exclusion_list.push_back(chosen_core);
			}
		}

		// Update the core utilization
		core_util[chosen_core] = core_util[chosen_core] + task_util;
	}

	// Assign tasks without self suspensions
	for (unsigned int index = 0; index < wfd_ordered_tasks.size(); index++)
	{		
		// Find core with min util
		if (wfd_ordered_tasks[index].getTotalGe() != 0)
			continue;

		sched_flag = 1;
		exclusion_list.clear();
		double task_util = (wfd_ordered_tasks[index].getC()+wfd_ordered_tasks[index].getTotalGm())/wfd_ordered_tasks[index].getT();

		// Try cores until schedulable
		while (sched_flag != 0)
		{
			// Find core with min util
			chosen_core = find_minutil_core_excluding(core_util, 0, exclusion_list);

			// Allocate the task to that core
			wfd_ordered_tasks[index].setCoreID(chosen_core);

			// Add the task to the vector of tasks
			if (sched_flag == 1)
				wfd_mapped_tasks.push_back(wfd_ordered_tasks[index]);

			// Sort Tasks according to RMS
			std::sort(wfd_mapped_tasks.begin(), wfd_mapped_tasks.end(), priority_ordering);

			// Check Schedulability
			sched_flag = check_schedulability(wfd_mapped_tasks, sched_mode, resp_time_rd, 
										      resp_time_jd, req_blocking_rd, job_blocking_jd);

			// If not schedulable declare an unfeasible partition
			if (sched_flag != 0)
			{
				if (exclusion_list.size() == num_cores)
					return -1;
				else
					exclusion_list.push_back(chosen_core);
			}
		}

		// Update the core utilization
		core_util[chosen_core] = core_util[chosen_core] + task_util;
	}

	// Copy over vector with core mappings 
	task_vector = wfd_mapped_tasks;

	return 0;
}


