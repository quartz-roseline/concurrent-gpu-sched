/*
 * @file paper_examples.cpp
 * @brief Uniprocessor Examples found in the paper
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

// Comparator class for ordering tasks using RMS
struct CompareTaskPriorityRMS {
    bool operator()(Task const & t1, Task const & t2) {
        // return "true" if "t1" has higher RMS priority than "t2"
        return t1.getT() < t2.getT();
    }
} ComparePriorityRMS;


int main(int argc, char **argv)
{
	std::vector<Task> task_vector, scaled_task_vector;
	double best_frequency_csc, best_frequency_csa, best_frequency_csi;
	double best_cpufreq_ctg, best_gpufreq_ctg, best_cpufreq_cte, best_gpufreq_cte;
	double best_frequency_up, best_frequency_low;
	double energy_csc, energy_csa, energy_csi, energy_ctg, energy_ctb;
	double true_gpu_util, true_cpu_util;
	int number_tasks;
	int retval;
	int number_gpu_tasks;
	int sched_flag = 0;
	int post_sched_flag = 0;
	int taskset_counter = 0;

	// Epsilon convergence factor
	double epsilon = 0.01;
	if (argc > 1)
		epsilon = std::atof(argv[1]);

	// Example to Consider
	int example_case;
	if (argc > 2)
		epsilon = std::atof(argv[2]);

	// Energy Constants
	if (argc > 5)
	{
		// Flip the fraction sweep mode if we have are doing an energy sweep
		if (set_energy_constants(std::atof(argv[3]),std::atof(argv[4]),std::atof(argv[5])) < 0)
		{
			std::cout << "Negative energy constants not allowed\n";
			return -1;
		}
	}

	/* initialize the taskset */
	task_t task1;
	task_t task2;
	task1.C = 10; task1.Ge = 8; task1.Gm = 0; task1.D = 50; task1.T = 50;
	task2.C = 20; task2.Ge = 5; task2.Gm = 0; task2.D = 80; task2.T = 80;

	Task t1(task1);
	Task t2(task2);

	task_vector.push_back(t1);
	task_vector.push_back(t2);

	// Sort Vector based on Some Priority ordering (here RMS)
	std::sort(task_vector.begin(), task_vector.end(), ComparePriorityRMS);

	// Print Taskset
	print_taskset(task_vector);

	// Check Schedulability
	sched_flag = check_schedulability_request_driven(task_vector);

	// If taskset is schedulable then compute frequency
	if (sched_flag == 0)
	{
		// Compute utilization values for energy calculations
		true_cpu_util = get_taskset_cpu_util(task_vector);
		true_gpu_util = get_taskset_gpu_util(task_vector);

		// Initialize Variables
		best_frequency_up = get_taskset_cpu_util(task_vector);
		best_frequency_low = best_frequency_up;

		// CycleSolo-CPU
		auto start_c = std::chrono::high_resolution_clock::now();
		retval = cycle_solo_cpu(task_vector, &best_frequency_up, &best_frequency_low);
		best_frequency_csc = binary_search_cpu_frequency_range(task_vector, epsilon, 1.0, best_frequency_up, best_frequency_low);
		energy_csc = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csc, 1.0);
		auto finish_c = std::chrono::high_resolution_clock::now();

		std::cout << "CycleSolo-CPU: " << "Best CPU Frequency = " << best_frequency_csc << "\n";
		std::cin.get();

		// Initialize Variables
		best_frequency_up = get_taskset_gpu_util(task_vector);
		best_frequency_low = best_frequency_up;

		// CycleSolo-Accel
		start_c = std::chrono::high_resolution_clock::now();
		retval = cycle_solo_accel(task_vector, &best_frequency_up, &best_frequency_low);
		best_frequency_csa = binary_search_gpu_frequency_range(task_vector, epsilon, 1.0, best_frequency_up, best_frequency_low);
		energy_csa = calculate_energy(true_cpu_util, true_gpu_util, 1.0, best_frequency_csa);
		finish_c = std::chrono::high_resolution_clock::now();

		std::cout << "CycleSolo-Accel: " << "Best Accelerator Frequency = " << best_frequency_csa << "\n";
		std::cin.get();

		// Initialize Variables
		best_frequency_up = get_taskset_cpu_util(task_vector);
		best_frequency_low = best_frequency_up;

		// CycleSolo-ID
		start_c = std::chrono::high_resolution_clock::now();
		retval = cycle_solo_id(task_vector, &best_frequency_up, &best_frequency_low);
		best_frequency_csi = binary_search_common_frequency_range(task_vector, epsilon, best_frequency_up, best_frequency_low);
		energy_csi = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csi, best_frequency_csi);
		finish_c = std::chrono::high_resolution_clock::now();

		std::cout << "CycleSolo-ID: " << "Best ID Frequency = " << best_frequency_csi << "\n";
		std::cin.get();

		// Initialize Variables
		best_cpufreq_ctg = best_frequency_csc;
		best_gpufreq_ctg = best_frequency_csa;

		// CycleTandem Greedy Search
		start_c = std::chrono::high_resolution_clock::now();
		retval = cycle_tandem(task_vector, &best_cpufreq_ctg, &best_gpufreq_ctg, epsilon);
		energy_ctg = calculate_energy(true_cpu_util, true_gpu_util, best_cpufreq_ctg, best_gpufreq_ctg);
		finish_c = std::chrono::high_resolution_clock::now();

		// Initialize Variables
		best_cpufreq_cte = best_frequency_csc;
		best_gpufreq_cte = best_frequency_csa;

		// CycleTandem Greedy Search
		start_c = std::chrono::high_resolution_clock::now();
		retval = cycle_tandem_bruteforce(task_vector, &best_cpufreq_cte, &best_gpufreq_cte, epsilon);
		energy_ctb = calculate_energy(true_cpu_util, true_gpu_util, best_cpufreq_cte, best_gpufreq_cte);
		finish_c = std::chrono::high_resolution_clock::now();

		std::cout << "CycleTandem-Brute: " << "\n";
		std::cout << "Best CPU Frequency         = " << best_cpufreq_cte << "\n";
		std::cout << "Best Accelerator Frequency = " << best_gpufreq_cte << "\n";
		std::cin.get();

		taskset_counter++;
	}

	return 0;
}