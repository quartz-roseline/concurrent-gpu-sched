/*
 * @file task.hpp
 * @brief Task class header
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

#ifndef TASK_HPP
#define TASK_HPP

#include <vector>

typedef struct gpu_params {
	double Gm;		// WCET CPU intervention
	double Ge;		// WCET GPU execution
	double F;       // Fraction of the GPU used (between 0 and 1)
} gpu_params_t;

typedef struct task {
	double C;						  // WCET on CPU	
	std::vector<gpu_params_t> G;      // GPU WCET Parameters
	double D;						  // Deadline < Period
	double T;						  // Period
} task_t;

class Task
{
	// Constructor and destructor
	public: Task(task_t task_params);
	public: ~Task();

	// Get the task parameters
	public: double getC() const; 
	public: double getD() const; 
	public: double getT() const;

	// Get the GPU Exec Parameters (i -> segment number, starting at 0)
	public: double getGm(unsigned int i) const; 
	public: double getGe(unsigned int i) const; 
	public: double getF(unsigned int i) const; 
	public: double getG(unsigned int i) const; 
	public:	unsigned int getNumGPUSegments() const;
	public: double getTotalGm() const; 
	public: double getTotalGe() const;
	public: double getTotalG() const; 

	// Get the maximum CPU intervention, and max CPU intervention with fractional req <= fraction
	public:	double getMaxGm() const;
	public:	double getMaxGmLeqFraction(double fraction) const;

	// Get the maximum task GPU fractional requirement
	public: double getMaxF() const;

	// Get the maximum task GPU fractional requirement after index
	public: double getIndexMaxF(unsigned int index, unsigned int &max_index) const;

	// Get the total CPU time required by the task in the period
	public: double getE() const;

	// Get/Set the WCRT of the GPU segments, Get the Max WCRT and Total WCRT
	// -> Use these functions with care, their initial values may be garbage, set all H before using
	public: double getH(unsigned int i) const;
	public:	int setH(unsigned int i, double H);
	public:	double getMaxH() const;
	public: double getTotalH() const;

	// Get physical core and energy parameters	
	public: double getCpuFreq() const; 
	public: double getGpuFreq() const; 
	public: unsigned int getCoreID() const; 

	// Set the core to which the task is allocated
	public: int setCoreID(unsigned int coreID);

	// Scale the task parameters by the frequency scaling factor
	public: int scale_cpu(double cpu_frequency);
	public: int scale_gpu(double gpu_frequency);

	// Convert the timescale multiply and floor (to remove floating point errors)
	public: int task_timescale(int scaling_factor); 

	// Private Variables
	private: task_t params;
	private: unsigned int num_gpu_segments;
	private: double cpu_freq;
	private: double gpu_freq;
	private: int core_id; 
	private: std::vector<double> gpu_seg_response_time;  // WCRT of the GPU segments
};

#endif
