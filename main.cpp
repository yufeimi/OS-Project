#include "process.h"
#include "schedule_algorithm.h"
#include <assert.h>
#include <cstring>
#include <fstream>
#include <math.h>
#include <vector>

std::vector<process> process_generator(const int, const double, const int,
                                       const int);

int main(int argc, char const *argv[]) {
  /* argv[1] is s as the random number seed
     argv[2] is lambda for exponential distribution of interarrival time
     argv[3] is the upper bound for valid pseudo-random numbers
     argv[4] is n as the number of processes to simulate
     argv[5] is t_cs as time for context switch. (positive even number)
     argv[6] is alpha for estimation of next CPU burst time.
     argv[7] is t_slice as the time slice value
     argv[8] is rr_add is either BEGGINING or END. END is default
  */
  if (argc < 8) {
    std::cerr << "Usage: ./main <seed> <lambda> <upper bound>"
              << " <n> <t_cs> <alpha> <t_slice> <rr_add>(optional)\n";
    return 1;
  }
  int s = atoi(argv[1]);
  double lambda = atof(argv[2]);
  int threshold = atoi(argv[3]);
  int n = atoi(argv[4]);
  int t_cs = atoi(argv[5]);
  double alpha = atof(argv[6]);
  int t_slice = atoi(argv[7]);
  bool rr_add = false;
  if (argc == 9) {
    if (strcmp(argv[8], "END") == 0) {
      rr_add = false;
    } else if (strcmp(argv[8], "BEGGINING") == 0) {
      rr_add = true;
    } else {
      std::cerr << "Usage: ./main <s> <lambda> <upper bound>"
                << " <n> <t_cs> <alpha> <t_slice> <rr_add>(optional)\n";
      return 1;
    }
  }
  std::vector<process> processes = process_generator(s, lambda, threshold, n);
  // std::vector<process> processes;
  // process A(400, 'A', {10, 20, 30, 60, 480});
  // process B(0, 'B', {500, 20, 15, 30, 500});
  // processes.push_back(A);
  // processes.push_back(B);
  SJF_scheduling SJF_simulator(processes, t_cs, lambda, alpha);
  SJF_simulator.run();
  std::cout << std::endl;
  SRT_scheduling SRT_simulator(processes, t_cs, lambda, alpha);
  SRT_simulator.run();
  std::cout << std::endl;
  FCFS_scheduling FCFS_simulator(processes, t_cs);
  FCFS_simulator.run();
  std::cout << std::endl;
  RR_scheduling RR_simulator(processes, t_cs, t_slice, rr_add);
  RR_simulator.run();

  // Write stats to file
  std::ofstream file("simout.txt");
  file << "Algorithm SJF\n";
  SJF_simulator.write_stats(file);
  file << "Algorithm SRT\n";
  SRT_simulator.write_stats(file);
  file << "Algorithm FCFS\n";
  FCFS_simulator.write_stats(file);
  file << "Algorithm RR\n";
  RR_simulator.write_stats(file);
  file.close();
  return 0;
}

std::vector<process> process_generator(const int s, const double lambda,
                                       const int threshold, const int n) {
  // Initialize the random number table with given seed
  srand48(s);
  // Initialize the map for storing processes.
  std::vector<process> processes;
  // Initialize the process ID
  char process_ID = 'A';
  // no more than 26 processes to simulate
  assert(n > 0 && n < 27);
  for (int i = 0; i < n; ++i) {
    double r = drand48();
    int arrival_time = (int)(-log(r) / lambda);
    if (arrival_time > threshold) {
      --i;
      continue;
    }
    r = drand48();
    int n_cpu_bursts = (int)(r * 100) + 1;
    std::vector<int> time_sequence;
    time_sequence.resize(n_cpu_bursts * 2 - 1);
    for (int j = 0; j < n_cpu_bursts; ++j) {
      r = drand48();
      int cpu_time = threshold + 1;
      while (cpu_time > threshold) {
        cpu_time = (int)ceil(-log(r) / lambda);
        if (cpu_time > threshold)
          r = drand48();
      }
      time_sequence[2 * j] = cpu_time;
      if (j == n_cpu_bursts - 1)
        break;
      r = drand48();
      int io_time = threshold + 1;
      while (io_time > threshold) {
        io_time = (int)ceil(-log(r) / lambda);
        if (io_time > threshold)
          r = drand48();
      }
      time_sequence[2 * j + 1] = io_time;
    }
    process tmp_process(arrival_time, process_ID, time_sequence);
    processes.push_back(tmp_process);
    ++process_ID;
  }
  return processes;
}
