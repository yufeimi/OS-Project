#include "process.h"
#include "schedule_algorithm.h"
#include <assert.h>
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
     argv[7] is t_sice as the time slice value
     argv[8] is rr_add is either BEGGINING or END. END is default
  */
  // std::vector<process> processes = process_generator(70, 0.001, 3000, 10);
  std::vector<process> processes;
  process A(5, 'A', {10, 20, 30, 60, 480});
  process B(3,'B', {5, 25, 15, 30, 500});
  processes.push_back(A);
  processes.push_back(B);
  RR_scheduling RRsimulator(processes, 4, 100, false);
  //SJF_scheduling SJFsimulator(processes, 4,0.001,0.5);
  RRsimulator.run();
  //SJFsimulator.run();
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