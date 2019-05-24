/*
 * @file request-driven-test-conc.hpp
 * @brief Header of the request-driven schedulability test for concurrent GPUs
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

#ifndef REQUEST_DRIVEN_CONC_HPP
#define REQUEST_DRIVEN_CONC_HPP

#include <vector>

#include "task.hpp"
#include "config.hpp"

/**************** Calculate Schedulability using the Request-Driven Approach ********************/ 
/* Params: task_vector: vector of tasks 
		   resp_time: vector of response times of each task (is populated on the return)
		   req_blocking: vector of vectors of per-request direct blocking faced (populated by the function)
		   simple_flag: true indicates perform the simple schedulability test
   Returns: 0 if schedulable */
int check_schedulability_request_driven_conc(std::vector<Task> &task_vector, 
										std::vector<double> &resp_time,
										std::vector<std::vector<double>> &req_blocking,
										bool simple_flag);

#endif
