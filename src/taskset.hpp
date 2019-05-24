/*
 * @file taskset.hpp
 * @brief Taskset operations helper functions header
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

#ifndef TASKSET_HPP
#define TASKSET_HPP

#include <vector>

#include "task.hpp"

/**************** Print all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: 0 if schedulable */
void print_taskset(const std::vector<Task> &task_vector);

/**************** Scale the task set CPU & GPU frequency by the given factor ********************/ 
/* Params: task_vector: vector of tasks 
		   cpu_frequency: cpu frequency scaling factor
		   gpu_frequency: gpu frequency scaling factor
   Returns: new task vector */
std::vector<Task> scale_taskset_frequency(const std::vector<Task> &task_vector, double cpu_frequency, double gpu_frequency);

/**************** Get the CPU utilization all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: cpu utilization */
double get_taskset_cpu_util(const std::vector<Task> &task_vector);

/**************** Get the CPU utilization GPU-using tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: cpu utilization of GPU-using tasks */
double get_gputasks_cpu_util(const std::vector<Task> &task_vector);

/**************** Get the GPU utilization all the tasks in the taskset *************************/
/* Params: task_vector: vector of tasks 
   Returns: gpu utilization */
double get_taskset_gpu_util(const std::vector<Task> &task_vector);

/* Find index of max lp task with largest GPU segment */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: index of low-priority task with max GPU segment */
int find_max_lp_gpu_index(int index, const std::vector<Task> &task_vector);

/* Find the length of the lp task with largest GPU segment */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: length of the max low-priority task GPU segment */
double find_max_lp_gpu_segment(int index, const std::vector<Task> &task_vector);

/* Find the length of the lp task with largest GPU segment in terms of WCRT */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_max_lp_gpu_wcrt_segment(int index, const std::vector<Task> &task_vector);

/* Find tindex of max lp task with largest GPU segment in terms of WCRT */
/* Params: index      : task index in vector ordered by priority
           task_vector: vector of tasks 
   Returns: index of low-priority task with max WCRT low-priority task GPU segment */
int find_max_lp_gpu_wcrt_index(int index, const std::vector<Task> &task_vector);

/* Find the length of the lp task with largest GPU segment in terms of WCRT, which is smaller than value */
/* Params: index      : task index in vector ordered by priority
		   value      : value which wcrt should be less than or equal to
		   lp_index   : the index of the said task (populated by this function)
		   num_biggest: the kth largest element to find
           task_vector: vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_next_max_lp_gpu_wcrt_segment(int index, double value, int &lp_index, int num_biggest, const std::vector<Task> &task_vector);

/* Find the length of the lp task with largest GPU segment in terms of WCRT, which is smaller than value and has larger fraction */
/* Params: index       : task index in vector ordered by priority
		   value       : value which wcrt should be less than or equal to
		   num_biggest : the kth largest element to find (starting at 1st)
		   req_fraction: the fraction of the chosen request (populated by this function)
		   fraction    : smallest fraction request to consider
           task_vector : vector of tasks 
   Returns: length of the max WCRT low-priority task GPU segment */
double find_next_max_lp_gpu_wcrt_segment_frac(int index, double value, int num_biggest, double &req_fraction, double fraction, const std::vector<Task> &task_vector);

/* Find the kth largest cpu internvention of a lp task which is smaller (<=) than value */
/* Params: index      : task index in vector ordered by priority which we need to search
		   value      : value which wcrt should be less than or equal to
		   num_biggest: the kth largest element to find (starting at 1st)
           task_vector: vector of tasks 
   Returns: length of the kth max cpu intervention segment of a low-priority task GPU segment */
double find_next_task_max_gpu_intervention_segment(int index, double value, int num_biggest, const std::vector<Task> &task_vector);

/**************** Calculate upper bound on number of instance of low-prio tasks in the response time ********************/ 
/* Params: lp_task       : the low-prio task class we want to operate on
		   response_time : the time in which we want to see how many effective number of instance show up
   Returns: effective number of instances of the low-prio task which show up */
unsigned int getTheta(const Task &lp_task, double response_time);

#endif
