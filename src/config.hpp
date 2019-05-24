/*
 * @file config.hpp
 * @brief Configuration Header
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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEBUG 0

// Taskset Generation Configuration
#define CPU_TASK_UPPER_BOUND 0.4	/* Upper bound on the CPU task utilization */
#define GPU_TASK_UPPER_BOUND 0.4    /* Upper bound on the GPU task utilization */
#define CPU_INTERVENTION_UTIL 0.1   /* Upper bound on the CPU intervention required for the GPU */
#define CPU_INTERVENTION_BOUND 1    /* Time upper bound on the CPU intervention of the GPU segment */
#define MIN_PERIOD 5				/* Minimum Task Period */
#define MAX_PERIOD 500				/* Maximum Task Period */
#define MAX_TASKS 10			    /* Maximum number of tasks */
#define MAX_TASKS_MC4 15            /* Maximum tasks for 4 cores */
#define GPU_FRACTION_GRANULARITY 10 /* Maximum segments the GPU can be broken into */
#define MAX_GPU_SEGMENTS 5          /* Maximum number of GPU segments */
#define MAX_GPU_FRACTION 1.0        /* Maximum GPU Fraction           */
#define FRACTION_TASKS_GPU 0.5      /* Fraction of tasks with GPU segments */

// Floating point errors overflow compensation
#define EPSILON_FLO 0.001           /* Term to compensate for ceil floor floating point errors*/


#endif
