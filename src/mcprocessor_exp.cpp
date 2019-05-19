/*
 * @file mcprocessor_exp.cpp
 * @brief Multi-core processor Experiments
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
	std::vector<Task> task_vector, scaled_task_vector;
	double best_frequency_csc, best_frequency_csa, best_frequency_csi;
	double best_cpufreq_ctg, best_gpufreq_ctg, best_cpufreq_cte, best_gpufreq_cte;
	double best_frequency_up, best_frequency_low;
	double energy_csc, energy_csa, energy_csi, energy_ctg, energy_ctb;
	double sa_energy_csc, sa_energy_csa, sa_energy_csi, sa_energy_ctg, sa_energy_ctb;
	double true_gpu_util, true_cpu_util;
	int number_tasks;
	int retval;
	int number_gpu_tasks;
	int sched_flag_wfd = 0;
	int sched_flag_sawfd = 0;
	int post_sched_flag = 0;
	int wfd_taskset_counter = 0;
	int sa_wfd_taskset_counter = 0;
	int common_counter=0;

	// Average Energy Counters
	double avg_energy_csc = 0, avg_energy_csa = 0, avg_energy_csi = 0, avg_energy_ctg = 0, avg_energy_ctb = 0;

	// SA-WFD Average Energy Counters
	double sa_avg_energy_csc = 0, sa_avg_energy_csa = 0, sa_avg_energy_csi = 0, sa_avg_energy_ctg = 0, sa_avg_energy_ctb = 0;

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

	// Energy Constants
	if (argc > 11)
	{
		// Flip the fraction sweep mode if we have are doing an energy sweep
		fraction_sweep_mode = 0;
		if (set_energy_constants(std::atof(argv[9]),std::atof(argv[10]),std::atof(argv[11])) < 0)
		{
			std::cout << "Negative energy constants not allowed\n";
			return -1;
		}
	}

	/* initialize random seed: */
  	srand (time(NULL));

	while (wfd_taskset_counter < taskset_count && sa_wfd_taskset_counter < taskset_count)
	{
		if (fraction_sweep_mode == 1)
		{
			number_tasks = MAX_TASKS_MC4;
			number_gpu_tasks = floor(gpu_task_fraction*number_tasks);
		}
		else
		{
			number_tasks = (rand() % MAX_TASKS_MC4) + ceil(utilization_bound/CPU_TASK_UPPER_BOUND);
			number_gpu_tasks = ceil(gpu_task_fraction*number_tasks); // Guarantees minimum fraction of tasks as specified
		}

		task_vector = generate_tasks(number_tasks, number_gpu_tasks, utilization_bound, gpu_utilization_bound, harmonic_flag);

		// If Task Vector is empty the try again
		if (task_vector.empty())
			continue;

		std::cout << "Tasket " << wfd_taskset_counter << " " << sa_wfd_taskset_counter << "\n";

		// Sort Vector based on Some Priority ordering (here RMS)
		std::sort(task_vector.begin(), task_vector.end(), ComparePriorityRMS);

		// Create a Partition
		sched_flag_wfd = worst_fit_decreasing(task_vector, num_cores, ComparePriorityRMS);

		// If taskset is schedulable then compute frequency
		if (sched_flag_wfd == 0)
		{
			// Compute utilization values for energy calculations
			true_cpu_util = get_taskset_cpu_util(task_vector);
			true_gpu_util = get_taskset_gpu_util(task_vector);

			// Initialize Variables
			std::vector<double> best_freq_up_vec(num_cores, get_taskset_cpu_util(task_vector)/num_cores);
			std::vector<double> best_freq_low_vec(num_cores, get_taskset_cpu_util(task_vector)/num_cores);
			double best_frequency_up, best_frequency_low;

			// CycleSolo-CPU
			auto start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_cpu_mc(task_vector, best_freq_up_vec, best_freq_low_vec, 1);
			best_frequency_csc = binary_search_cpu_frequency_range(task_vector, epsilon, 1.0, best_freq_up_vec[0], best_freq_low_vec[0]);
			energy_csc = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csc, 1.0);
			auto finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_frequency_up = get_taskset_gpu_util(task_vector);
			best_frequency_low = best_frequency_up;

			// CycleSolo-Accel
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_accel_mc(task_vector, &best_frequency_up, &best_frequency_low);
			best_frequency_csa = binary_search_gpu_frequency_range(task_vector, epsilon, 1.0, best_frequency_up, best_frequency_low);
			energy_csa = calculate_energy(true_cpu_util, true_gpu_util, 1.0, best_frequency_csa);
			finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_frequency_up = get_taskset_cpu_util(task_vector)/num_cores;
			best_frequency_low = best_frequency_up;

			// CycleSolo-ID
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_id_mc(task_vector, &best_frequency_up, &best_frequency_low);
			best_frequency_csi = binary_search_common_frequency_range(task_vector, epsilon, best_frequency_up, best_frequency_low);
			energy_csi = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csi, best_frequency_csi);
			finish_c = std::chrono::high_resolution_clock::now();

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

			wfd_taskset_counter++;
		}

		// Create a Partition
		sched_flag_sawfd = sync_aware_worst_fit_decreasing(task_vector, num_cores, ComparePriorityRMS);

		// If taskset is schedulable then compute frequency
		if (sched_flag_sawfd == 0 )
		{
			// Compute utilization values for energy calculations
			true_cpu_util = get_taskset_cpu_util(task_vector);
			true_gpu_util = get_taskset_gpu_util(task_vector);

			// Initialize Variables
			std::vector<double> best_freq_up_vec(num_cores, get_taskset_cpu_util(task_vector)/num_cores);
			std::vector<double> best_freq_low_vec(num_cores, get_taskset_cpu_util(task_vector)/num_cores);
			double best_frequency_up, best_frequency_low;

			// CycleSolo-CPU
			auto start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_cpu_mc(task_vector, best_freq_up_vec, best_freq_low_vec, 1);
			best_frequency_csc = binary_search_cpu_frequency_range(task_vector, epsilon, 1.0, best_freq_up_vec[0], best_freq_low_vec[0]);
			sa_energy_csc = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csc, 1.0);
			auto finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_frequency_up = get_taskset_gpu_util(task_vector);
			best_frequency_low = best_frequency_up;

			// CycleSolo-Accel
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_accel_mc(task_vector, &best_frequency_up, &best_frequency_low);
			best_frequency_csa = binary_search_gpu_frequency_range(task_vector, epsilon, 1.0, best_frequency_up, best_frequency_low);
			sa_energy_csa = calculate_energy(true_cpu_util, true_gpu_util, 1.0, best_frequency_csa);
			finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_frequency_up = get_taskset_cpu_util(task_vector)/num_cores;
			best_frequency_low = best_frequency_up;

			// CycleSolo-ID
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_solo_id_mc(task_vector, &best_frequency_up, &best_frequency_low);
			best_frequency_csi = binary_search_common_frequency_range(task_vector, epsilon, best_frequency_up, best_frequency_low);
			sa_energy_csi = calculate_energy(true_cpu_util, true_gpu_util, best_frequency_csi, best_frequency_csi);
			finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_cpufreq_ctg = best_frequency_csc;
			best_gpufreq_ctg = best_frequency_csa;

			// CycleTandem Greedy Search
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_tandem(task_vector, &best_cpufreq_ctg, &best_gpufreq_ctg, epsilon);
			sa_energy_ctg = calculate_energy(true_cpu_util, true_gpu_util, best_cpufreq_ctg, best_gpufreq_ctg);
			finish_c = std::chrono::high_resolution_clock::now();

			// Initialize Variables
			best_cpufreq_cte = best_frequency_csc;
			best_gpufreq_cte = best_frequency_csa;

			// CycleTandem Greedy Search
			start_c = std::chrono::high_resolution_clock::now();
			retval = cycle_tandem_bruteforce(task_vector, &best_cpufreq_cte, &best_gpufreq_cte, epsilon);
			sa_energy_ctb = calculate_energy(true_cpu_util, true_gpu_util, best_cpufreq_cte, best_gpufreq_cte);
			finish_c = std::chrono::high_resolution_clock::now();

			sa_wfd_taskset_counter++;
		}
		// Update the Average Energy Values
		if (sched_flag_wfd == 0 && sched_flag_sawfd == 0)
		{
			avg_energy_csc = avg_energy_csc + energy_csc;
			avg_energy_csa = avg_energy_csa + energy_csa; 
			avg_energy_csi = avg_energy_csi + energy_csi; 
			avg_energy_ctg = avg_energy_ctg + energy_ctg;
			avg_energy_ctb = avg_energy_ctb + energy_ctb;

			sa_avg_energy_csc = sa_avg_energy_csc + sa_energy_csc;
			sa_avg_energy_csa = sa_avg_energy_csa + sa_energy_csa; 
			sa_avg_energy_csi = sa_avg_energy_csi + sa_energy_csi; 
			sa_avg_energy_ctg = sa_avg_energy_ctg + sa_energy_ctg;
			sa_avg_energy_ctb = sa_avg_energy_ctb + sa_energy_ctb;
			common_counter++;
		}
	}

	// Update the Average Energy Values
	avg_energy_csc = avg_energy_csc/common_counter;
	avg_energy_csa = avg_energy_csa/common_counter; 
	avg_energy_csi = avg_energy_csi/common_counter; 
	avg_energy_ctg = avg_energy_ctg/common_counter;
	avg_energy_ctb = avg_energy_ctb/common_counter;

	sa_avg_energy_csc = sa_avg_energy_csc/common_counter;
	sa_avg_energy_csa = sa_avg_energy_csa/common_counter; 
	sa_avg_energy_csi = sa_avg_energy_csi/common_counter; 
	sa_avg_energy_ctg = sa_avg_energy_ctg/common_counter;
	sa_avg_energy_ctb = sa_avg_energy_ctb/common_counter;

	// Write values to file
	if (file_flag == 1)
	{
		outfile << wfd_taskset_counter << ","
				<< avg_energy_csc << ","
				<< avg_energy_csa << ","
				<< avg_energy_csi << ","
				<< avg_energy_ctg << ","
				<< avg_energy_ctb << "\n";
		outfile << sa_wfd_taskset_counter << ","
				<< sa_avg_energy_csc << ","
				<< sa_avg_energy_csa << ","
				<< sa_avg_energy_csi << ","
				<< sa_avg_energy_ctg << ","
				<< sa_avg_energy_ctb << "\n";
		outfile.close();
	}

	return 0;
}