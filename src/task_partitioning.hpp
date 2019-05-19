/*
 * @file task_partitioning.hpp
 * @brief WFD-based Task Partitioning Header
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
#ifndef TASK_PARTITION_HPP
#define TASK_PARTITION_HPP

/* Standard Library Imports */
#include <vector>
#include <functional>

/* Internal Headers */
#include "task.hpp"

/**************** The WFD Partitioning Algorithm ********************/ 
/* Params: task_vector       : vector of tasks 
		   num_cores         : number of cores
		   priority_ordering : std::sort operator specifying priority ordering of tasks
   Returns: 0 if a feasible partition exists */
int worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, std::function<bool(Task const &, Task const &)> priority_ordering);

/**************** The Synchronization-Aware WFD Partitioning Algorithm ********************/ 
/* Params: task_vector       : vector of tasks 
		   num_cores         : number of cores
		   priority_ordering : std::sort operator specifying priority ordering of tasks
   Returns: 0 if a feasible partition exists */
int sync_aware_worst_fit_decreasing(std::vector<Task> &task_vector, int num_cores, std::function<bool(Task const &, Task const &)> priority_ordering);

#endif


