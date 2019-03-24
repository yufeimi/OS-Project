#include "schedule_algorithm.h"

bool resolveTie(process_ptr i, process_ptr j) {
  return (i->get_ID() < j->get_ID());
}

bool ShorterJobTime(process_ptr a, process_ptr b) {
  if (a->get_last_estimated_burst_time() < b->get_last_estimated_burst_time()) {
    return true;
  } else if (a->get_last_estimated_burst_time() ==
             b->get_last_estimated_burst_time()) {
    return (resolveTie(a, b));
  } else {
    return false;
  }
}

bool ShorterRemainingTime(process_ptr a, process_ptr b) {
  if (a->get_estimated_remaining_time() < b->get_estimated_remaining_time()) {
    return true;
  } else if (a->get_estimated_remaining_time() ==
             b->get_estimated_remaining_time()) {
    return (resolveTie(a, b));
  } else {
    return false;
  }
}

schedule_algorithm::schedule_algorithm(const std::vector<process> &p,
                                       const int t_cs)
    : processes(p), t_cs(t_cs), time(0), running(processes.end()), wait_time(0),
      n_wait(0), turnaround_time(0), n_cs(0), n_preemption(0) {
  assert(t_cs % 2 == 0);
}

void schedule_algorithm::write_stats(std::ofstream &file) {
  // compute CPU burst time
  double CPU_burst_time = 0;
  double CPU_num = 0;
  for (auto i : processes) {
    for (unsigned int j = 0; j < i.get_time_sequence().size(); ++j) {
      if (j % 2 == 0) {
        CPU_burst_time += i.get_time_sequence()[j];
        CPU_num += 1;
      }
    }
  }

  // Output
  file << std::setprecision(3) << std::fixed;
  file << "-- average CPU burst time: " << CPU_burst_time / CPU_num << " ms\n"
       << "-- average wait time: " << wait_time / CPU_num << " ms\n"
       << "-- average turnaround time: " << turnaround_time / CPU_num << " ms\n"
       << "-- total number of context switches: " << n_cs << "\n"
       << "-- total number of preemptions: " << n_preemption << "\n";
}

void schedule_algorithm::print_overview() {
  for (auto i : processes) {
    i.print_overview();
  }
}

void schedule_algorithm::context_switch(process_ptr process_in) {
  // Calculate turnaround time for process that is exiting
  if (running != processes.end()) {
    turnaround_time += t_cs / 2;
  }

  if (running != processes.end() && running->get_state() == 0) {
    std::stringstream event;
    event << "Process " << running->get_ID()
          << " switching out of CPU; will block on I/O until time "
          << (running->get_remaining_time() + time + t_cs / 2) << "ms";
    print_event(event.str());
  }

  bool process_in_wait = false;
  // Check if this process_in is in ready queue
  if (process_in == *(ready_queue.begin())) {
    process_in_wait = true;
  }

  ++time;
  int start_time = 1;
  // First half of context switch
  for (int i = start_time; i < t_cs / 2; i++) {
    if (running == processes.end()) {
      break;
    } else {
      start_time = 0;
    }
    // check if any new processes have the same arrival time.
    check_arrival();
    // Process_out is not in the ready queue for the first half
    running->wait_for_1ms(false);
    // Process in I/O burst proceed for t_cs
    do_blocking();
    perform_add_to_ready_queue();
    // Processes in ready queue wait for t_cs
    do_waiting();
    time++;
  }

  if (running != processes.end()) {
    // Remove the running process
    // Determine whether add the running process to ready_queue
    // or block on I/O, or terminate it.
    if (running->get_state() == 1) {
      prepare_add_to_ready_queue(running);
    } else if (running->get_state() == 0) {
      blocked.insert(running);
    } else if (running->get_state() == -1) {
      terminated.insert(running);
    }
  }

  // Check if the process in has changed since the switch out
  if (process_in == processes.end()) {
    running = processes.end();
    return;
  }

  // Note extra turnaround time should be added if this is NOT
  // the initial context switch!
  for (int i = start_time; i < t_cs / 2; i++) {
    // check if any new processes have the same arrival time.
    check_arrival();
    // Process in I/O burst proceed for t_cs
    do_blocking();
    if (i > start_time || start_time == 0) {
      perform_add_to_ready_queue();
    }
    // Do these i the first loop
    if (i == start_time) {
      if (process_in_wait) {
        // Check if the process in has changed since the switch out
        if (process_in != *(ready_queue.begin())) {
          process_in = *(ready_queue.begin());
          // wait_time -= process_in->get_wait_time();
          // turnaround_time -= process_in->get_wait_time();
        }
        ready_queue.pop_front();
      }
      // Replace the running process
      running = process_in;
      // Calculate time for incoming process
      turnaround_time += t_cs / 2;
    } else {
      // Process_in is not in the ready queue for the second half
      running->wait_for_1ms(false);
    }
    if (i == start_time && start_time == 1) {
      perform_add_to_ready_queue();
    }
    // Processes in ready queue wait for t_cs
    do_waiting();
    time++;
  }
  std::stringstream event;
  if (!running->preempted()) {
    event << "Process " << running->get_ID() << " started using the CPU for "
          << running->get_remaining_time() << "ms burst";
  } else {
    event << "Process " << running->get_ID() << " started using the CPU with "
          << running->get_remaining_time() << "ms remaining";
  }
  ++n_cs;
  print_event(event.str());
}

void schedule_algorithm::check_arrival() {
  for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
    if (itr->get_arrival_time() == time) {
      prepare_add_to_ready_queue(itr);
    }
  }
  for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
    // If the I/O time is end then move it to ready_queue
    if ((*itr)->get_state() == 1) {
      blocked.erase(itr);
      prepare_add_to_ready_queue(*itr);
    }
  }
}

void schedule_algorithm::do_waiting() {
  for (auto itr = ready_queue.begin(); itr != ready_queue.end(); ++itr) {
    if (running == processes.end() && itr == ready_queue.begin()) {
      continue;
    }
    (*itr)->wait_for_1ms(true);
    wait_time += 1;
    turnaround_time += 1;
  }
}

void schedule_algorithm::do_blocking() {
  for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
    (*itr)->block_for_1ms();
  }
}

void schedule_algorithm::prepare_add_to_ready_queue(
    process_ptr process_to_add) {
  pre_ready_queue.push_back(process_to_add);
};

void schedule_algorithm::print_event(std::string event) {
  if (time < 1000 || event.find("terminated") != std::string::npos ||
      event.find("Simulator") != std::string::npos) {
    std::cout << "time " << time << "ms: " << event << " [Q";
    for (auto i : ready_queue) {
      std::cout << " " << i->get_ID();
    }
    if (ready_queue.size() == 0)
      std::cout << " <empty>";
    std::cout << "]\n";
  }
}

FCFS_scheduling::FCFS_scheduling(const std::vector<process> &p, const int t_cs)
    : schedule_algorithm(p, t_cs) {}

void FCFS_scheduling::run() {
  print_overview();
  print_event("Simulator started for FCFS");
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    if (state == 0) {
      std::stringstream event;
      std::string plural =
          running->get_remaining_CPU_bursts() > 1 ? " bursts " : " burst ";
      event << "Process " << running->get_ID() << " completed a CPU burst; "
            << running->get_remaining_CPU_bursts() << plural << "to go";
      print_event(event.str());
    } else if (state == -1) {
      std::stringstream event;
      event << "Process " << running->get_ID() << " terminated";
      print_event(event.str());
    }
    // check if any new processes have the same arrival time.
    check_arrival();
    // block processes on I/O for 1ms
    do_blocking();
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // all processes in ready queue wait for 1ms
    do_waiting();

    // Determine context switch
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
        cs = 1;
        state = 1;
      } else if (state != -2) {
        context_switch(processes.end());
        cs = 1;
        state = -2;
      }
    }
    if (cs == 1) {
      // time does not increment after context switch
      cs = 0;
      continue;
    }
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    if (running != processes.end()) {
      state = running->run_for_1ms();
      turnaround_time += 1;
    } else {
      // no current running process
      state = -2;
    }
    // time increment
    ++time;
  }
  print_event("Simulator ended for FCFS");
}

void FCFS_scheduling::perform_add_to_ready_queue() {
  std::sort(pre_ready_queue.begin(), pre_ready_queue.end(), resolveTie);
  for (auto i : pre_ready_queue) {
    ready_queue.push_back(i);
    std::stringstream event;
    if (i->get_arrival_time() == time) {
      event << "Process " << i->get_ID() << " arrived; added to ready queue";
    } else {
      event << "Process " << i->get_ID()
            << " completed I/O; added to ready queue";
    }
    print_event(event.str());
    n_wait += 1;
  }
  pre_ready_queue.clear();
}

RR_scheduling::RR_scheduling(const std::vector<process> &p, const int t_cs,
                             const int t_slice, const bool add)
    : schedule_algorithm(p, t_cs), t_slice(t_slice), add(add) {}

void RR_scheduling::run() {
  print_overview();
  print_event("Simulator started for RR");
  // The time the current process is running for
  int time_running = 0;
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    if (state == 0) {
      std::stringstream event;
      std::string plural =
          running->get_remaining_CPU_bursts() > 1 ? " bursts " : " burst ";
      event << "Process " << running->get_ID() << " completed a CPU burst; "
            << running->get_remaining_CPU_bursts() << plural << "to go";
      print_event(event.str());
    } else if (state == -1) {
      std::stringstream event;
      event << "Process " << running->get_ID() << " terminated";
      print_event(event.str());
    }
    // check if any new processes have the same arrival time.
    check_arrival();
    // block processes on I/O for 1ms
    do_blocking();
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // all processes in ready queue wait for 1ms
    do_waiting();
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
        cs = 1;
        state = 1;
        time_running = 0;
      } else if (state != -2) {
        context_switch(processes.end());
        cs = 1;
        state = -2;
        time_running = 0;
      }
    }
    // when time slice expires
    if (time_running >= t_slice && !ready_queue.empty()) {
      std::stringstream event;
      event << "Time slice expired; process " << running->get_ID()
            << " preempted with " << running->get_remaining_time()
            << "ms to go";
      print_event(event.str());
      context_switch(*(ready_queue.begin()));
      time_running = 0;
      cs = 1;
      state = 1;
      ++n_preemption;
    } else if (time_running >= t_slice) {
      std::stringstream event;
      event << "Time slice expired; no preemption because ready queue is empty";
      print_event(event.str());
      time_running = 0;
    }
    if (cs == 1) {
      // time does not increment after context switch
      cs = 0;
      continue;
    }
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    if (running != processes.end()) {
      state = running->run_for_1ms();
      turnaround_time += 1;
      ++time_running;
    } else {
      // no current running process
      state = -2;
    }
    time++;
  }
  print_event("Simulator ended for RR");
}

void RR_scheduling::perform_add_to_ready_queue() {
  std::sort(pre_ready_queue.begin(), pre_ready_queue.end(), resolveTie);
  for (auto i : pre_ready_queue) {
    if (add == true) {
      ready_queue.push_front(i);
    } else {
      ready_queue.push_back(i);
    }
    if (i->preempted()) {
      continue;
    }
    n_wait += 1;
    std::stringstream event;
    if (i->get_arrival_time() == time) {
      event << "Process " << i->get_ID() << " arrived; added to ready queue";
    } else {
      event << "Process " << i->get_ID()
            << " completed I/O; added to ready queue";
    }
    print_event(event.str());
  }
  pre_ready_queue.clear();
}

SJF_scheduling::SJF_scheduling(const std::vector<process> &p, const int t_cs,
                               const double lambda, const double alpha)
    : schedule_algorithm(p, t_cs), lambda(lambda), alpha(alpha) {}

void SJF_scheduling::run() {
  print_overview();
  print_event("Simulator started for SJF");
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    if (state == 0) {
      std::stringstream event1, event2;
      std::string plural =
          running->get_remaining_CPU_bursts() > 1 ? " bursts " : " burst ";
      event1 << "Process " << running->get_ID() << " completed a CPU burst; "
             << running->get_remaining_CPU_bursts() << plural << "to go";
      print_event(event1.str());
      // Recalculate tau for the process that completes its burst
      int tau = est_tau(running->get_last_estimated_burst_time(),
                        running->get_last_burst_time());
      running->set_estimated_remaining_time(tau);
      event2 << "Recalculated tau = " << tau << "ms for process "
             << running->get_ID();
      print_event(event2.str());
    } else if (state == -1) {
      std::stringstream event;
      event << "Process " << running->get_ID() << " terminated";
      print_event(event.str());
    }
    // check if have any new processes have the same arrival time.
    check_arrival();
    // block processes on I/O for 1ms
    do_blocking();
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // all processes in ready queue wait for 1ms
    do_waiting();
    // Determine context switch
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
        cs = 1;
        state = 1;
      } else if (state != -2) {
        context_switch(processes.end());
        cs = 1;
        state = -2;
      }
    }
    if (cs == 1) {
      // time does not increment after context switch
      cs = 0;
      continue;
    }
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    if (running != processes.end()) {
      state = running->run_for_1ms();
      turnaround_time += 1;
    } else {
      // no current running process
      state = -2;
    }
    // time increment
    ++time;
  }
  print_event("Simulator ended for SJF");
}
void SJF_scheduling::perform_add_to_ready_queue() {
  for (auto i : pre_ready_queue) {
    ready_queue.push_back(i);
    std::stringstream event;
    if (i->get_arrival_time() == time) {
      // Set tau0 for new process
      i->set_estimated_remaining_time(1 / lambda);
      ready_queue.sort(ShorterJobTime);
      event << "Process " << i->get_ID() << " (tau "
            << i->get_last_estimated_burst_time() << "ms)"
            << " arrived; added to ready queue";
    } else {
      ready_queue.sort(ShorterJobTime);
      event << "Process " << i->get_ID() << " (tau "
            << i->get_last_estimated_burst_time() << "ms)"
            << " completed I/O; added to ready queue";
    }
    print_event(event.str());
    n_wait += 1;
  }
  pre_ready_queue.clear();
}

int SJF_scheduling::est_tau(double tau, int t) {
  int next_est = (int)ceil(alpha * t + (1 - alpha) * tau);
  return next_est;
}

SRT_scheduling::SRT_scheduling(const std::vector<process> &p, const int t_cs,
                               const double lambda, const double alpha)
    : schedule_algorithm(p, t_cs), lambda(lambda), alpha(alpha),
      preempting_process(processes.end()) {}

void SRT_scheduling::run() {
  print_overview();
  print_event("Simulator started for SRT");
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    if (state == 0) {
      std::stringstream event1, event2;
      std::string plural =
          running->get_remaining_CPU_bursts() > 1 ? " bursts " : " burst ";
      event1 << "Process " << running->get_ID() << " completed a CPU burst; "
             << running->get_remaining_CPU_bursts() << plural << "to go";
      print_event(event1.str());
      int tau = est_tau(running->get_last_estimated_burst_time(),
                        running->get_last_burst_time());
      running->set_estimated_remaining_time(tau);
      event2 << "Recalculated tau = " << tau << "ms for process "
             << running->get_ID();
      print_event(event2.str());
    } else if (state == -1) {
      std::stringstream event;
      event << "Process " << running->get_ID() << " terminated";
      print_event(event.str());
    }
    // check if any new processes have the same arrival time.
    check_arrival();
    // block processes on I/O for 1ms
    do_blocking();
    // remove the running process
    // Determine whether add the running process to ready_queue
    // or blocked by its state
    preempting_process = check_preemption();
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // all processes in ready queue wait for 1ms
    do_waiting();
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
        while (!ready_queue.empty() &&
               ShorterRemainingTime(*(ready_queue.begin()), running)) {
          ready_queue_preemption();
          ++n_preemption;
        }
        cs = 1;
        state = 1;
      } else if (state != -2) {
        context_switch(processes.end());
        cs = 1;
        state = -2;
      }
    }
    // when preemption happens
    if (preempting_process != processes.end()) {
      ++n_preemption;
      context_switch(preempting_process);
      while (!ready_queue.empty() &&
             ShorterRemainingTime(*(ready_queue.begin()), running)) {
        ready_queue_preemption();
        ++n_preemption;
      }
      cs = 1;
      state = 1;
    }
    if (cs == 1) {
      // time does not increment after context switch
      cs = 0;
      continue;
    }
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    if (running != processes.end()) {
      state = running->run_for_1ms();
      turnaround_time += 1;
    } else {
      // no current running process
      state = -2;
    }
    time++;
  }
  print_event("Simulator ended for SRT");
}

void SRT_scheduling::perform_add_to_ready_queue() {
  for (auto i : pre_ready_queue) {
    if (i == processes.end()) {
      continue;
    }
    if (i != preempting_process) {
      n_wait += 1;
    }
    ready_queue.push_back(i);
    std::stringstream event;
    if (i->preempted() || i == running || i == preempting_process) {
      ready_queue.sort(ShorterRemainingTime);
      continue;
    }
    if (i->get_arrival_time() == time) {
      // Set tau0 for new process
      i->set_estimated_remaining_time(1 / lambda);
      ready_queue.sort(ShorterRemainingTime);
      event << "Process " << i->get_ID() << " (tau "
            << i->get_last_estimated_burst_time() << "ms)"
            << " arrived; added to ready queue";
    } else {
      ready_queue.sort(ShorterRemainingTime);
      event << "Process " << i->get_ID() << " (tau "
            << i->get_last_estimated_burst_time() << "ms)"
            << " completed I/O; added to ready queue";
    }
    print_event(event.str());
  }
  pre_ready_queue.clear();
}

int SRT_scheduling::est_tau(double tau, int t) {
  int next_est = (int)ceil(alpha * t + (1 - alpha) * tau);
  return next_est;
}

process_ptr SRT_scheduling::check_preemption() {
  if (running == processes.end() || running->get_state() != 1) {
    return processes.end();
  }
  int remaining_time = running->get_estimated_remaining_time();
  process_ptr return_value = processes.end();
  for (auto i : pre_ready_queue) {
    if (i->get_arrival_time() == time) {
      // Set tau0 for new process
      i->set_estimated_remaining_time(1 / lambda);
    }
    // Compare remainiing time and current remaining time
    if (i->get_estimated_remaining_time() < remaining_time) {
      remaining_time = i->get_estimated_remaining_time();
      return_value = i;
    }
  }
  // remove the preempting process by replacing it by null itr
  if (return_value != processes.end()) {
    for (auto &i : pre_ready_queue) {
      if (i == return_value) {
        i = processes.end();
      }
    }
    ready_queue.push_back(return_value);
    ready_queue.sort(ShorterRemainingTime);
    std::stringstream event;
    if (time == return_value->get_arrival_time()) {
      event << "Process " << return_value->get_ID() << " (tau "
            << return_value->get_estimated_remaining_time()
            << "ms) will preempt " << running->get_ID();
    } else {
      event << "Process " << return_value->get_ID() << " (tau "
            << return_value->get_estimated_remaining_time()
            << "ms) completed I/O and will preempt " << running->get_ID();
    }
    print_event(event.str());
  }
  return return_value;
}

void SRT_scheduling::ready_queue_preemption() {
  process_ptr preempting_process = ready_queue.front();
  int remaining_time = preempting_process->get_estimated_remaining_time();
  // check if have any new processes have the same arrival time.
  check_arrival();
  // block processes on I/O for 1ms
  do_blocking();

  for (auto i : pre_ready_queue) {
    if (i->get_arrival_time() == time) {
      // Set tau0 for new process
      i->set_estimated_remaining_time(1 / lambda);
    }
    // Compare remainiing time and current remaining time
    if (i->get_estimated_remaining_time() < remaining_time) {
      remaining_time = i->get_estimated_remaining_time();
      preempting_process = i;
    }
  }

  std::stringstream event;
  for (auto &i : pre_ready_queue) {
    if (i == preempting_process) {
      i = processes.end();
      ready_queue.push_back(preempting_process);
      ready_queue.sort(ShorterRemainingTime);
    }
    if (time == preempting_process->get_arrival_time()) {
      event << "Process " << preempting_process->get_ID() << " (tau "
            << preempting_process->get_estimated_remaining_time()
            << "ms) will preempt " << running->get_ID();
    } else {
      event << "Process " << preempting_process->get_ID() << " (tau "
            << preempting_process->get_estimated_remaining_time()
            << "ms) completed I/O and will preempt " << running->get_ID();
    }
  }

  if (preempting_process == *(ready_queue.begin())) {
    event << "Process " << preempting_process->get_ID() << " (tau "
          << preempting_process->get_estimated_remaining_time()
          << "ms) will preempt " << running->get_ID();
  }
  print_event(event.str());

  perform_add_to_ready_queue();

  do_waiting();

  context_switch(preempting_process);
}
