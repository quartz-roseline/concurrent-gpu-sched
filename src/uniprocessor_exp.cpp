/*
 * @file uniprocessor_exp.cpp
 * @brief Uniprocessor Experiments
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
#include "config.hpp"

/* Schedulability test headers */
#include "request-driven-test.hpp"
#include "job-driven-test.hpp"
#include "hybrid-test.hpp"
#include "request-driven-test-conc.hpp"
#include "job-driven-test-conc.hpp"
#include "hybrid-test-conc.hpp"

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
	double true_gpu_util, true_cpu_util;
	int number_tasks;
	int retval;
	int number_gpu_tasks;
	int taskset_counter = 0;

	// Schedulability Flags
	int sched_flag_rd = 0;
	int sched_flag_jd = 0;
	int sched_flag_hybrid = 0;
	int sched_flag_rd_conc = 0;
	int sched_flag_rd_conc_simple = 0;
	int sched_flag_jd_conc = 0;
	int sched_flag_jd_conc_ro = 0;
	int sched_flag_hybrid_conc = 0;

	// Schedulability Counters
	int counter_rd = 0;
	int counter_jd = 0;
	int counter_hybrid = 0;
	int counter_rd_conc = 0;
	int counter_rd_conc_simple = 0;
	int counter_jd_conc = 0;
	int counter_jd_conc_ro = 0;
	int counter_hybrid_conc = 0;

	// Average Util counters
	double average_cpu_util = 0, average_gpu_util = 0;

	// Output Filestream
	std::ofstream outfile;
	int file_flag = 0;

	// Number of tasksets to simulate
	int taskset_count = 1;
	if (argc > 1)
		taskset_count = std::atoi(argv[1]);

	// Generate only harmonic tasksets
	int harmonic_flag = 0;
	if (argc > 2)
		harmonic_flag = std::atoi(argv[2]);

	// Output filename
	if (argc > 3)
	{
		std::string filename(argv[3]);
		file_flag = 1;
		outfile.open(filename, std::ios_base::app);
	}

	// Epsilon convergence factor
	double epsilon = 0.01;
	if (argc > 4)
		epsilon = std::atof(argv[4]);

	// CPU Utilization Bound
	double utilization_bound = 0.5;
	if (argc > 5)
		utilization_bound = std::atof(argv[5]);

	// GPU utilization bound
	double gpu_utilization_bound = 0.3;
	if (argc > 6)
		gpu_utilization_bound = std::atof(argv[6]);

	// Fraction of tasks with GPU segments
	double gpu_task_fraction = 0.5;
	int fraction_sweep_mode = 0;
	if (argc > 7)
	{
		gpu_task_fraction = std::atof(argv[7]);
		fraction_sweep_mode = 1;
	}

	// Max Number of GPU segments
	int number_gpu_segments = MAX_GPU_SEGMENTS;
	if (argc > 8)
	{
		number_gpu_segments = std::atoi(argv[8]);
	}

	// Request-Driven Vectors
	std::vector<double> resp_time_rd;
	std::vector<std::vector<double>> req_blocking_rd;

	// Job-Driven Vectors
	std::vector<double> resp_time_jd;
	std::vector<double> job_blocking_jd;

	/* initialize random seed: */
  	srand (time(NULL));

	while (taskset_counter < taskset_count)
	{
		if (fraction_sweep_mode == 1)
		{
			number_tasks = MAX_TASKS;
			number_gpu_tasks = floor(gpu_task_fraction*number_tasks);
		}
		else
		{
			number_tasks = (rand() % MAX_TASKS) + 1;
			number_gpu_tasks = ceil(gpu_task_fraction*number_tasks); // Guarantees minimum fraction of tasks as specified
		}

		std::cout << "Taskset " << taskset_counter << " NumTasks = " << number_tasks << " NumAccTasks = " << number_gpu_tasks<< std::endl;
		task_vector = generate_tasks(number_tasks, number_gpu_tasks, number_gpu_segments, utilization_bound, gpu_utilization_bound, harmonic_flag);

		// If Task Vector is empty the try again
		if (task_vector.empty())
			continue;

		// Sort Vector based on Some Priority ordering (here RMS)
		std::sort(task_vector.begin(), task_vector.end(), ComparePriorityRMS);

		print_taskset(task_vector);

		// Check Schedulability -> Non-concurrent approaches
		resp_time_jd.clear();
		req_blocking_rd.clear();
		resp_time_jd.clear();
		sched_flag_rd = check_schedulability_request_driven(task_vector, resp_time_rd, req_blocking_rd);
		sched_flag_jd = check_schedulability_job_driven(task_vector, resp_time_jd);
		sched_flag_hybrid = check_schedulability_hybrid(task_vector, resp_time_rd, resp_time_jd, req_blocking_rd);
		
		// Check Schedulability -> Concurrent approaches (simple)
		resp_time_jd.clear();
		req_blocking_rd.clear();
		resp_time_jd.clear();
		job_blocking_jd.clear();
		sched_flag_rd_conc_simple = check_schedulability_request_driven_conc(task_vector, resp_time_rd, req_blocking_rd, true);
		sched_flag_jd_conc = check_schedulability_job_driven_conc(task_vector, resp_time_jd, job_blocking_jd, false);
		
		// Check Schedulability -> Concurrent approaches (complex)
		resp_time_jd.clear();
		req_blocking_rd.clear();
		resp_time_jd.clear();
		job_blocking_jd.clear();
		sched_flag_rd_conc = check_schedulability_request_driven_conc(task_vector, resp_time_rd, req_blocking_rd, false);
		sched_flag_jd_conc_ro = check_schedulability_job_driven_conc(task_vector, resp_time_jd, job_blocking_jd, true);
		sched_flag_hybrid_conc = check_schedulability_hybrid_conc(task_vector, resp_time_rd, resp_time_jd, req_blocking_rd, job_blocking_jd);

		std::cout << "Schedulability:" << "\n";
		std::cout << "Request-Driven        : " << sched_flag_rd << "\n";
		std::cout << "Job-Driven            : " << sched_flag_jd << "\n";
		std::cout << "Hybrid                : " << sched_flag_hybrid << "\n";
		std::cout << "Request-Driven-Conc-S : " << sched_flag_rd_conc_simple << "\n";
		std::cout << "Job-Driven-Conc       : " << sched_flag_jd_conc << "\n";
		std::cout << "Request-Driven-Conc   : " << sched_flag_rd_conc << "\n";
		std::cout << "Job-Driven-Conc-RO    : " << sched_flag_jd_conc_ro << "\n";
		std::cout << "Hybrid-Conc           : " << sched_flag_hybrid_conc << "\n";
		// std::cin.get();

		// Update the schedulability counters
		if (sched_flag_rd == 0)
			counter_rd++;

		if (sched_flag_jd == 0)
			counter_jd++;

		if (sched_flag_hybrid == 0)
			counter_hybrid++;

		if (sched_flag_rd_conc == 0)
			counter_rd_conc++;

		if (sched_flag_jd_conc == 0)
			counter_jd_conc++;

		if (sched_flag_rd_conc_simple == 0)
			counter_rd_conc_simple++;

		if (sched_flag_jd_conc_ro == 0)
			counter_jd_conc_ro++;

		if (sched_flag_hybrid_conc == 0)
			counter_hybrid_conc++;
		
		// Compute utilization values for energy calculations
		true_cpu_util = get_taskset_cpu_util(task_vector);
		true_gpu_util = get_taskset_gpu_util(task_vector);

		// Update average utilization values
		average_gpu_util = average_gpu_util + true_gpu_util;
		average_cpu_util = average_cpu_util + true_cpu_util;
		taskset_counter++;
	}

	// Compute the Average
	average_gpu_util = average_gpu_util/taskset_count;
	average_cpu_util = average_cpu_util/taskset_count;

	// Write values to file
	if (file_flag == 1)
	{	
		outfile << average_cpu_util << ","
		        << average_gpu_util << ","
		        << taskset_count << ","
		        << counter_rd << ","
		        << counter_jd << ","
		        << counter_hybrid << ","
		        << counter_rd_conc << ","
		        << counter_jd_conc << ","
		        << counter_rd_conc_simple << ","
		        << counter_jd_conc_ro << ","
		        << counter_hybrid_conc;
		outfile.close();
	}

	std::cout << "Tasksets: " << taskset_count << "\n";
	std::cout << "Avg. CPU Util :" << average_cpu_util << "\n";
	std::cout << "Avg. GPU Util :" << average_gpu_util << "\n";
	std::cout << "Request-Driven        : " << counter_rd << "\n";
	std::cout << "Job-Driven            : " << counter_jd << "\n";
	std::cout << "Hybrid                : " << counter_hybrid << "\n";
	std::cout << "Request-Driven-Conc-S : " << counter_rd_conc_simple << "\n";
	std::cout << "Job-Driven-Conc       : " << counter_jd_conc << "\n";
	std::cout << "Request-Driven-Conc   : " << counter_rd_conc << "\n";
	std::cout << "Job-Driven-Conc-RO    : " << counter_jd_conc_ro << "\n";
	std::cout << "Hybrid-Conc           : " << counter_hybrid_conc << "\n";

	return 0;
}