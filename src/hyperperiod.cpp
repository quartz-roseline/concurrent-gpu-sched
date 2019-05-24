/*
 * @file request-driven-test.cpp
 * @brief Implementing to calculate the hyperperiod for tasks with uint64_teger periods
 * @author Anon 
 * 
 * Copyright (c) Anon, 2019. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, 
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
 * DATA, OR PROFITS; OR BUSINESS uint64_tERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <cmath>

#include "hyperperiod.hpp"

uint64_t gcd(uint64_t a, uint64_t b)
{
    for (;;)
    {
        if (a == 0) return b;
        b %= a;
        if (b == 0) return a;
        a %= b;
    }
}

uint64_t period_lcm(uint64_t a, Task const & t2)
{
    uint64_t b = (uint64_t) floor(t2.getT());

    uint64_t temp = gcd(a, b);

    return temp ? ((a/temp) * b) : 0;
}

/**************** Calculate Task Set hyperperiod ********************/ 
uint64_t compute_hyperperiod(const std::vector<Task> &task_vector)
{
    uint64_t result = 1;

    for (unsigned int i = 0; i < task_vector.size(); i++)
    {
        result = period_lcm(result, task_vector[i]);
    } 

    return result;
}

/**************** Calculate CPU execution time in the hyperperiod ********************/ 
double compute_cputime_hyperperiod(const std::vector<Task> &task_vector)
{
    uint64_t hyperperiod = compute_hyperperiod(task_vector);
    double cputime = 0;
    int num_gpu_segments;

    for (unsigned int i = 0; i < task_vector.size(); i++)
    {
        cputime = cputime + (task_vector[i].getC())*(hyperperiod/floor(task_vector[i].getT()));
        num_gpu_segments = task_vector[index].getNumGPUSegments();
        if (num_gpu_segments != 0)
        {
            for (unsigned int j = 0; j < num_gpu_segments; j++)
            {
                cputime = cputime + (task_vector[i].getGm(j))*(hyperperiod/floor(task_vector[i].getT()));
            }
        }
    }

    return cputime;
}

/**************** Calculate GPU execution time in the hyperperiod ********************/ 
double compute_gputime_hyperperiod(const std::vector<Task> &task_vector)
{
    uint64_t hyperperiod = compute_hyperperiod(task_vector);
    double gputime = 0;
    int num_gpu_segments;

    for (unsigned int i = 0; i < task_vector.size(); i++)
    {
        num_gpu_segments = task_vector[index].getNumGPUSegments();
        if (num_gpu_segments != 0)
        {
            for (unsigned int j = 0; j < num_gpu_segments; j++)
            {
                gputime = gputime + (task_vector[i].getGe(j))*(hyperperiod/floor(task_vector[i].getT()));
            }
        }
    }

    return gputime;
}