#include "schedule_algorithm.h"

bool resolveTie(process_ptr i, process_ptr j) {
  return (i->get_ID() < j->get_ID());
}

schedule_algorithm::schedule_algorithm(const std::vector<process> &p,
                                       const int t_cs)
    : processes(p), t_cs(t_cs), time(0), running(processes.end()) {
  assert(t_cs % 2 == 0);
}

void schedule_algorithm::context_switch(process_ptr process_in) {
  bool process_in_wait = false;
  // Check if this process_in is in ready queue
  if (process_in == *(ready_queue.begin())) {
    process_in_wait = true;
    ready_queue.pop_front();
  }

  if (running != processes.end() && running->get_state() == 0) {
    std::stringstream event;
    event << "Process " << running->get_ID()
          << " switching out of CPU; will block on I/O until time "
          << (running->get_remaining_time() + time) << "ms";
    print_event(event.str());
  }

  // First half of context switch
  for (int i = 0; i < t_cs / 2; i++) {
    if (running == processes.end())
      break;
    time++;
    // Processes in ready queue wait for t_cs
    for (auto itr = ready_queue.begin(); itr != ready_queue.end(); ++itr) {
      (*itr)->wait_for_1ms(true);
    }
    // Process_in waits for t_cs if it was in ready queue
    if (process_in_wait) {
      process_in->wait_for_1ms(true);
    }
    // check if have any new processes have the same arrival time.
    for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
      if (itr->get_arrival_time() == time) {
        prepare_add_to_ready_queue(itr);
      }
    }
    // Process_out is not in the ready queue for the first half
    running->wait_for_1ms(false);
    // Process in I/O burst proceed for t_cs
    for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
      int state = (*itr)->block_for_1ms();
      // If the I/O time is end then move it to ready_queue
      if (state == 1) {
        blocked.erase(itr);
        prepare_add_to_ready_queue(*itr);
      }
    }
    // In the final loop, remove the running process
    if (i == t_cs / 2 - 1) {
      // Determine whether add the running process to ready_queue
      // or blocked by its state
      if (running->get_state() == 1)
        prepare_add_to_ready_queue(running);
      else if (running->get_state() == 0) {
        blocked.insert(running);
      } else {
        terminated.insert(running);
      }
    }
    perform_add_to_ready_queue();
  }

  // Replace the running process
  if (process_in != processes.end()) {
    running = process_in;
  } else {
    running = processes.end();
    return;
  }

  // Note extra turnaround time should be added if this is NOT
  // the initial context switch!
  for (int i = 0; i < t_cs / 2; i++) {
    time++;
    // Processes in ready queue wait for t_cs
    for (auto itr = ready_queue.begin(); itr != ready_queue.end(); ++itr) {
      (*itr)->wait_for_1ms(true);
    }
    // check if have any new processes have the same arrival time.
    for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
      if (itr->get_arrival_time() == time) {
        prepare_add_to_ready_queue(itr);
      }
    }
    // Process_in is not in the ready queue for the second half
    running->wait_for_1ms(false);
    // Process in I/O burst proceed for t_cs
    for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
      int state = (*itr)->block_for_1ms();
      // If the I/O time is end then move it to ready_queue
      if (state == 1) {
        blocked.erase(itr);
        prepare_add_to_ready_queue(*itr);
      }
    }
    perform_add_to_ready_queue();
  }
  std::stringstream event;
  event << "Process " << running->get_ID() << " started using the CPU for "
        << running->get_remaining_time() << " ms burst";
  print_event(event.str());
}

void schedule_algorithm::prepare_add_to_ready_queue(
    process_ptr process_to_add) {
  pre_ready_queue.push_back(process_to_add);
};

void schedule_algorithm::print_event(std::string event) {
  std::cout << "time " << time << "ms: " << event << " [Q ";
  for (auto i : ready_queue) {
    std::cout << i->get_ID() << " ";
  }
  if (ready_queue.size() == 0)
    std::cout << "<empty>";
  std::cout << "]\n";
}

FCFS_scheduling::FCFS_scheduling(const std::vector<process> &p, const int t_cs)
    : schedule_algorithm(p, t_cs) {}

void FCFS_scheduling::run() {
  print_event("Simulator started for FCFS");
  while (terminated.size() < processes.size()) {
    /* each process in I/O burst proceed for 1ms  */
    for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
      int state = (*itr)->block_for_1ms(); // block_for_1ms() or wait_for_1ms ??
      if (state == 1) {
        blocked.erase(itr);
        prepare_add_to_ready_queue(*itr);
      }
    }
    // check if have any new processes have the same arrival time.
    for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
      if (itr->get_arrival_time() == time) {
        prepare_add_to_ready_queue(itr);
      }
    }
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    int state;
    if (running != processes.end()) {
      state = running->run_for_1ms();
      if (state == 0) {
        std::stringstream event;
        event << "Process " << running->get_ID() << " completed a CPU burst; "
              << running->get_remaining_CPU_bursts() << " to go";
        print_event(event.str());
      } else if (state == -1) {
        std::stringstream event;
        event << "Process " << running->get_ID() << " terminated";
        print_event(event.str());
      }
    } else {
      state = -2;
    }
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
      } else {
        context_switch(processes.end());
      }
    }
    time++;
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
  }
  pre_ready_queue.clear();
}

RR_scheduling::RR_scheduling(const std::vector<process> &p, const int t_cs,
                             const int t_slice, const bool add)
    : schedule_algorithm(p, t_cs), t_slice(t_slice), add(add) {}

void RR_scheduling::perform_add_to_ready_queue() {
  std::sort(pre_ready_queue.begin(), pre_ready_queue.end(), resolveTie);
  for (auto i : pre_ready_queue) {
    if (add == true) {
      ready_queue.push_front(i);
    } else {
      ready_queue.push_back(i);
    }
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

void RR_scheduling::run() {
  print_event("Simulator started for RR");

  // The time the current process is running for
  int time_running = 0;

  while (terminated.size() < processes.size()) {
    /* each process in I/O burst proceed for 1ms  */
    for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
      int state = (*itr)->block_for_1ms(); // block_for_1ms() or wait_for_1ms ??
      if (state == 1) {
        blocked.erase(itr);
        prepare_add_to_ready_queue(*itr);
      }
    }
    // check if have any new processes have the same arrival time.
    for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
      if (itr->get_arrival_time() == time) {
        prepare_add_to_ready_queue(itr);
      }
    }
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
    int state;
    if (running != processes.end()) {
      state = running->run_for_1ms();
      ++time_running;
      if (state == 0) {
        std::stringstream event;
        event << "Process " << running->get_ID() << " completed a CPU burst; "
              << running->get_remaining_CPU_bursts() << " to go";
        print_event(event.str());
      } else if (state == -1) {
        std::stringstream event;
        event << "Process " << running->get_ID() << " terminated";
        print_event(event.str());
      }
    } else {
      state = -2;
    }
    if (state != 1) {
      if (!ready_queue.empty()) {
        context_switch(*(ready_queue.begin()));
      } else {
        context_switch(processes.end());
      }
    }
    if (time_running >= t_slice && !ready_queue.empty()) {
      std::stringstream event;
      event << "Time slice expired; process " << running->get_ID()
            << " preempted with " << running->get_remaining_time()
            << " ms to go";
      print_event(event.str());
      context_switch(*(ready_queue.begin()));
      time_running = 0;
    }
    time++;
  }
  print_event("Simulator ended for RR");
}
