/*
 * @file task_partitioning.hpp
 * @brief WFD-based Task Partitioning Header
 * @author Anon 
 * 
 * Copyright (c) Anon, 2019. All rights reserved.
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
#ifndef TASK_PARTITION_HPP
#define TASK_PARTITION_HPP

/* Standard Library Imports */
#include <vector>
#include <functional>

/* Internal Headers */
#include "task.hpp"

/* Scheduling Type for Partitioning */
enum sched_type
{
	REQUEST_DRIVEN 					= 0,
	JOB_DRIVEN 						= 1,
	HYBRID 							= 2,
	REQUEST_DRIVEN_CONC_SIMPLE 		= 3,
	JOB_DRIVEN_CONC 				= 4,
	REQUEST_DRIVEN_CONC 			= 5,
	JOB_DRIVEN_CONC_RO 				= 6,
	HYBRID_CONC 					= 7,
	FIFO_CONC 						= 8,
	INVALID 						= 9
};

/**************** The WFD Partitioning Algorithm ********************/ 
/* Params: task_vector       : vector of tasks 
		   num_cores         : number of cores
		   sched_mode        : which schedulability test to use
		   resp_time_rd: vector of response times of each task (using the request-driven approach)
		   resp_time_jd: vector of response times of each task (using the job-driven approach)
		   req_blocking: direct blocking faced by individual requests (using the request-driven approach)
		   job_blocking: direct blocking faced by individual requests (using the job-driven approach)
		   priority_ordering : std::sort operator specifying priority ordering of tasks
   Returns: 0 if a feasible partition exists */
int worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, sched_type sched_mode,
						 std::vector<double> &resp_time_rd,
						 std::vector<double> &resp_time_jd,
						 std::vector<std::vector<double>> &req_blocking_rd,
						 std::vector<double> &job_blocking_jd,
						 std::function<bool(Task const &, Task const &)> priority_ordering);

/**************** The Synchronization-Aware WFD Partitioning Algorithm ********************/ 
/* Params: task_vector       : vector of tasks 
		   num_cores         : number of cores
		   sched_mode        : which schedulability test to use
		   resp_time_rd: vector of response times of each task (using the request-driven approach)
		   resp_time_jd: vector of response times of each task (using the job-driven approach)
		   req_blocking: direct blocking faced by individual requests (using the request-driven approach)
		   job_blocking: direct blocking faced by individual requests (using the job-driven approach)
		   priority_ordering : std::sort operator specifying priority ordering of tasks
   Returns: 0 if a feasible partition exists */
int sync_aware_worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, sched_type sched_mode,
						 			std::vector<double> &resp_time_rd,
									std::vector<double> &resp_time_jd,
									std::vector<std::vector<double>> &req_blocking_rd,
									std::vector<double> &job_blocking_jd,
									std::function<bool(Task const &, Task const &)> priority_ordering);
#endif


