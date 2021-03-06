/*
 * @file indirect-cis.hpp
 * @brief Header of the the indirect blocking and concurrency-induced serialization routines
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

#ifndef INDIRECT_CIS_HPP
#define INDIRECT_CIS_HPP

#include <vector>

#include "task.hpp"

/**************** Calculate Per-Request Concurrency-Induced Serialization sub-routine ********************/ 
/* Params: index: task index
		   req_index: index of gpu request within task 
		   task_vector: vector of tasks 
   Returns: the worst-case concurrency-induced serialization suffered by the task */
double calculate_request_cis(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector);

/**************** Calculate Per-Request Indirect Blocking sub-routine ********************/ 
/* Params: index: task index
		   req_index: index of gpu request within task 
		   task_vector: vector of tasks 
   Returns: the worst-case indirect blocking suffered by the task */
double calculate_request_indirect_blocking(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector);

/**************** Calculate the worst-case response time of a request sub-routine ********************/ 
/* Params: index: task index
		   req_index: index of gpu request within task 
		   task_vector: vector of tasks 
   Returns: the worst-case response time of each gpu request of the task */
double calculate_request_response_time(unsigned int index, unsigned int req_index, const std::vector<Task> &task_vector);

/**************** Calculate and set (in the Task class) the worst-case response time of all gpu requests sub-routine ********************/ 
/* Params: task_vector: vector of tasks 
   Returns: 0 if no errors */
int pre_compute_gpu_response_time(std::vector<Task> &task_vector);

#endif
