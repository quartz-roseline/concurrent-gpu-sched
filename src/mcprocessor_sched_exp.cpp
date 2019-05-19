/*
 * @file mcprocessor_sched_exp.cpp
 * @brief Multi-core processor Partitioning Schedulability Experiments
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
/* Standard Library Imports */
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>

/* Internal Headers */
#include "task.hpp"
#include "taskset.hpp"
#include "taskset-gen.hpp"
#include "request-driven-test.hpp"
#include "binary-search.hpp"
#include "config.hpp"
#include "energy.hpp"
#include "cycle-solo.hpp"
#include "cycle-tandem.hpp"
#include "task_partitioning.hpp"

// Comparator class for ordering tasks using RMS
struct CompareTaskPriorityRMS {
    bool operator()(Task const & t1, Task const & t2) {
        // return "true" if "t1" has higher RMS priority than "t2"
        return t1.getT() < t2.getT();
    }
} ComparePriorityRMS;


int main(int argc, char **argv)
{
	std::vector<Task> task_vector;
	int number_tasks;
	int retval;
	int number_gpu_tasks;
	int sched_flag = 0;
	int post_sched_flag = 0;
	int taskset_counter = 0;
	int wfd_sched_counter = 0;
	int sa_wfd_sched_counter = 0;


	// Output Filestream
	std::ofstream outfile;
	int file_flag = 0;

	// Number of tasksets to simulate
	int taskset_count = 1;
	if (argc > 1)
		taskset_count = std::atoi(argv[1]);

	// Number of Cores
	int num_cores = 1;
	if (argc > 2)
		num_cores = std::atoi(argv[2]);

	// Generate only harmonic tasksets
	int harmonic_flag = 0;
	if (argc > 3)
		harmonic_flag = std::atoi(argv[3]);

	// Output filename
	if (argc > 4)
	{
		std::string filename(argv[4]);
		file_flag = 1;
		outfile.open(filename, std::ios_base::app);
	}

	// Epsilon convergence factor
	double epsilon = 0.01;
	if (argc > 5)
		epsilon = std::atof(argv[5]);

	// CPU Utilization Bound
	double utilization_bound = 0.5;
	if (argc > 6)
		utilization_bound = std::atof(argv[6]);

	// GPU utilization bound
	double gpu_utilization_bound = 0.3;
	if (argc > 7)
		gpu_utilization_bound = std::atof(argv[7]);

	// Fraction of tasks with GPU segments
	double gpu_task_fraction = 0.5;
	int fraction_sweep_mode = 0;
	if (argc > 8)
	{
		gpu_task_fraction = std::atof(argv[8]);
		fraction_sweep_mode = 1;
	}

	/* initialize random seed: */
  	srand (time(NULL));

	while (taskset_counter < taskset_count)
	{
		if (fraction_sweep_mode == 1)
		{
			number_tasks = MAX_TASKS_MC4;
			number_gpu_tasks = floor(gpu_task_fraction*number_tasks);
		}
		else
		{
			number_tasks = (rand() % MAX_TASKS_MC4) + 1;	// 15 for 4 cores
			number_gpu_tasks = ceil(gpu_task_fraction*number_tasks); // Guarantees minimum fraction of tasks as specified
		}

		task_vector = generate_tasks(number_tasks, number_gpu_tasks, utilization_bound, gpu_utilization_bound, harmonic_flag);

		// If Task Vector is empty the try again
		if (task_vector.empty())
			continue;
		
		// Sort Vector based on Some Priority ordering (here RMS)
		std::sort(task_vector.begin(), task_vector.end(), ComparePriorityRMS);

		// Create a Partition using WFD
		sched_flag = worst_fit_decreasing(task_vector, num_cores, ComparePriorityRMS);

		if (sched_flag == 0)
			wfd_sched_counter++;

		// Create a Partition using SA-WFD
		sched_flag = sync_aware_worst_fit_decreasing(task_vector, num_cores, ComparePriorityRMS);

		if (sched_flag == 0)
			sa_wfd_sched_counter++;

		taskset_counter++;
	}

	std::cout << "Summary\n";
	std::cout << "WFD Schedulable    = " << 100*((double)wfd_sched_counter/taskset_counter) << "\n";
	std::cout << "SA-WFD Schedulable = " << 100*((double)sa_wfd_sched_counter/taskset_counter) << "\n";
	// Write values to file
	if (file_flag == 1)
	{
		outfile << ((double)wfd_sched_counter)/taskset_counter << ","
				<< ((double)sa_wfd_sched_counter)/taskset_counter << "\n";

		outfile.close();
	}

	return 0;
}