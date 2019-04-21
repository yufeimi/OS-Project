//  process.cpp
/* This class represents a process
 */
#ifndef PROCESS
#define PROCESS

#include <assert.h>
#include <iostream>
#include <vector>

class process {
public:
  // Default constructor
  process();
  // Copy constructor
  process(const process &);
  /* Brief constructor:
  A vector containing times for each burst is passed. The first
  time is for CPU burst, then I/O burst, and so on. The last one
  is also CPU burst, so the size must be odd.
  */
  process(const int, const char, const std::vector<int> &);
  // Return the process ID, denoted by a capital letter
  const char get_ID() const { return ID; };
  // Return whether the process is in CPU burst (1) or I/O burst (0)
  const int get_state() const { return state; };
  const int get_arrival_time() const { return arrival_time; };
  const int get_wait_time() const { return wait_time; };
  const int get_turnaround_time() const { return turnaround_time; };
  // Get remaining CPU burst for this burst. return 0 for blocked state
  const int get_remaining_time() const { return remaining_time; };
  // Get estimated remaining time that is set before. For SRT and SJF
  const int get_estimated_remaining_time() const {
    return estimated_remaining_time;
  };
  // Get last estimateed remianing time
  const int get_last_estimated_burst_time() const {
    return last_estimated_burst_time;
  };
  // Get last CPU burst time. For SRT and SJF
  const int get_last_burst_time() const;
  // Get remaining CPU bursts
  const int get_remaining_CPU_bursts() const;
  // return whether this process is preempted
  const int preempted() const;
  // Only for debugging. not changeable from outside
  const std::vector<int> &get_time_sequence() const { return time_sequence; };
  // Set the estimated remaining time
  void set_estimated_remaining_time(const int t);
  // reset wait time
  void reset_wait_time() { wait_time = 0; };
  /* Run for 1 ms. Returns the state after running 1ms:
  1 for CPU and 0 for IO. If the preocess ends then return -1*/
  const int run_for_1ms();
  /* Wait for 1 ms. increase wait_time and turnaround time
  according to its state. Increase turnaround time only when
  current time is at the beginning of the burst*/
  const int wait_for_1ms(bool);
  /*If it's blocked then proceed 1ms. Returns the state after blocking:
  1 for CPU and 0 for IO. */
  const int block_for_1ms();
  // Pinrt its burst time and io time
  void print();
  // Print arrival time and burst number only
  void print_overview();
  // Reset everything of this process
  void reset();

private:
  /* Proceed 1ms for the process, either in running or blocked.
  Return the state after proceeding (1 for CPU 0 for IO)*/
  const int proceed();
  // query state (1 for CPU, 0 for IO, -1 for end) given a time
  const int query_state(int) const;
  // Compute the remaining time.
  void compute_remaining_time();
  // The iterator indicating current position in the sequence.
  int current_time;
  // state. 1 for CPU 0 for I/O -1 for end
  int state;
  // The arrival time
  int arrival_time;
  // The total time (excluding waiting)
  int total_time;
  // Process ID
  char ID;
  // An int sequence for this process.
  std::vector<int> time_sequence;
  /* Wait time counter in ms for inquiry and output. Will be reset
  after every CPU busrt! */
  int wait_time;
  /* Turnaournd time counter in ms for inquiry and output. Will be reset
  after every CPU burst! */
  int turnaround_time;
  // Remaining time for this CPU burst. 0 when blocked
  int remaining_time;
  // Estimated remaining time sor SRT and SJF
  int estimated_remaining_time;
  // tau_i-1
  int last_estimated_burst_time;
};

#endif