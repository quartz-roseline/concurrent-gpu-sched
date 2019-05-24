/*
 * @file taskset-gen.hpp
 * @brief Taskset generation header
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

#ifndef TASKGEN_HPP
#define TASKGEN_HPP

#include <vector>

#include "task.hpp"
#include "config.hpp"

/**************** Generate a random taskset ********************/ 
/* Params: number_tasks: number of tasks
		   number_gpu_tasks: number of tasks with gpu sections
		   max_gpu_segments: maximum GPU segments per task
           utilization bound: taskset utilization desired
           gpu_utilization_bound: taskset utilization gpu bound 
           harmonic_flag: true indicates harmonic period
           gpu_seg_random_flag: true indicates generate number of per task gpu segments randomly using max
   		   max_gpu_fraction: maximum fraction of the GPU that a gpu request consumes
   Returns: Vector of Tasks, empty vector in case of error */
std::vector<Task> generate_tasks(int number_tasks, int number_gpu_tasks, int max_gpu_segments, double utilization_bound, double gpu_utilization_bound, int harmonic_flag, int gpu_seg_random_flag, double max_gpu_fraction);

#endif
