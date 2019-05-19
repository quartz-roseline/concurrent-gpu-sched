/*
 * @file task.cpp
 * @brief Task class implementation
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
#include <iostream>
#include <cmath>

// Include Internal Headers
#include "task.hpp"

// Constructor
Task::Task(task_t task_params)
{
	params = task_params;
	num_gpu_segments = task_params.G.size();
	cpu_freq = 1.0;
	gpu_freq = 1.0;
	core_id = 0;
	gpu_seg_response_time.reserve(num_gpu_segments);
}

// Destructor
Task::~Task()
{
	return;
}

// Get the task parameters
double Task::getC() const 
{
	return params.C;
}

double Task::getGm(unsigned int i) const 
{
	if (i >= num_gpu_segments)
		return -1;
	return params.G[i].Gm;
}

double Task::getGe(unsigned int i) const
{
	if (i >= num_gpu_segments)
		return -1;
	return params.G[i].Ge;
}

double Task::getF(unsigned int i) const
{
	if (i >= num_gpu_segments)
		return -1;
	return params.G[i].F;
}

double Task::getG(unsigned int i) const
{
	if (i >= num_gpu_segments)
		return -1;
	return params.G[i].Gm + params.G[i].Ge;
}

double Task::getTotalGm() const
{
	double totalGm = 0;

	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
			totalGm = totalGm + getGm(i);
	}

	return totalGm;
}

double Task::getTotalGe() const
{
	double totalGe = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
			totalGe = totalGe + getGe(i);
	}
	return totalGe;
}

double Task::getTotalG() const
{
	double totalG = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
			totalG = totalG + getGe(i) + getGm(i);
	}
	return totalG;
} 

double Task::getMaxGmLeqFraction(double fraction) const
{
	double maxGm = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
		{
			if (getGm(i) > maxGm && getF(i) <= fraction)
				maxGm = getGm(i);
		}
	}
	return maxGm;
}

double Task::getMaxGm() const
{
	return getMaxGmLeqFraction(1);
}

double Task::getMaxF() const
{
	double maxF = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
		{
			if (getF(i) > maxF)
				maxF = getF(i);
		}
	}
	return maxF;
}

double Task::getH(unsigned int i) const
{
	if (i >= num_gpu_segments)
		return -1;

	return gpu_seg_response_time[i];
}

int Task::setH(unsigned int i, double H)
{
	if (i >= num_gpu_segments)
		return -1;

	gpu_seg_response_time[i] = H;
	return 0;
}

double Task::getMaxH() const
{
	double maxH = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
		{
			if (getH(i) > maxH)
				maxH = getH(i);
		}
	}
	return maxH;
}

double Task::getTotalH() const
{
	double totalH = 0;
	if (num_gpu_segments > 0)
	{
		for (int i = 0; i < num_gpu_segments; i++)
		{
			totalH = getH(i);
		}
	}
	return totalH;
}

double Task::getE() const 
{
	return params.C + getTotalGm();
}

unsigned int Task::getNumGPUSegments() const
{
	return num_gpu_segments;
}

double Task::getD() const 
{
	return params.D;
}

double Task::getT() const 
{
	return params.T;
}

double Task::getCpuFreq() const
{
	return cpu_freq;
}

double Task::getGpuFreq() const
{
	return gpu_freq;
} 

unsigned int Task::getCoreID() const
{
	return core_id;
}

int Task::setCoreID(unsigned int coreID)
{
	core_id = coreID;
}

int Task::scale_cpu(double cpu_frequency)
{
	if (cpu_frequency > 1)
		return -1;

	// Check if current cpu frequency is 1
	if (cpu_freq == 1)
	{
		params.C = params.C/cpu_frequency;
		for (int i = 0; i < num_gpu_segments; i++)
			params.G[i].Gm = params.G[i].Gm*cpu_freq/cpu_frequency;
		cpu_freq = cpu_frequency;
	}
	else
	{
		// First scale to bring to frequency = 1, the apply new scaling factor
		params.C = params.C*cpu_freq/cpu_frequency;
		for (int i = 0; i < num_gpu_segments; i++)
			params.G[i].Gm = params.G[i].Gm*cpu_freq/cpu_frequency;
		cpu_freq = cpu_frequency;
	}
	return 0;
}

int Task::scale_gpu(double gpu_frequency)
{
	if (gpu_frequency > 1)
		return -1;

	// Check if current cpu frequency is 1
	if (gpu_freq == 1)
	{
		for (int i = 0; i < num_gpu_segments; i++)
			params.G[i].Ge = params.G[i].Ge/gpu_frequency;
		gpu_freq = gpu_frequency;
	}
	else
	{
		// First scale to bring to frequency = 1, the apply new scaling factor
		for (int i = 0; i < num_gpu_segments; i++)
			params.G[i].Ge = params.G[i].Ge*gpu_freq/gpu_frequency;
		gpu_freq = gpu_frequency;
	}
	return 0;
}

int Task::task_timescale(int scaling_factor)
{
	params.C = std::floor(params.C*scaling_factor);
	for (int i = 0; i < num_gpu_segments; i++)
	{
		params.G[i].Gm = std::floor(params.G[i].Gm*scaling_factor);
		params.G[i].Ge = std::floor(params.G[i].Ge*scaling_factor);
	}
	params.D = std::floor(params.D*scaling_factor);
	params.T = std::floor(params.T*scaling_factor);
	return 0;
}



