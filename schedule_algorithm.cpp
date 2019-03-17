#include "schedule_algorithm.h"

bool resolveTie(process_ptr i, process_ptr j) {
  return (i->get_ID() < j->get_ID());
}
bool compare_sjf(process_ptr a, process_ptr b){
  return ((a->get_last_estimated_burst_time()<b->get_last_estimated_burst_time())
            ||((a->get_last_estimated_burst_time()==b->get_last_estimated_burst_time())&&(a->get_ID()<b->get_ID())));
}
bool compare_srt(process_ptr a, process_ptr b){
  return (a->get_estimated_remaining_time()<b->get_estimated_remaining_time())||
          (a->get_estimated_remaining_time()==b->get_estimated_remaining_time()&&(a->get_ID()<b->get_ID()));
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

  ++time;
  int start_time = 1;
  // First half of context switch
  for (int i = start_time; i < t_cs / 2; i++) {
    if (running == processes.end()) {
      break;
    } else {
      start_time = 0;
    }
    // Processes in ready queue wait for t_cs
    do_waiting();
    // Process_in waits for t_cs if it was in ready queue
    if (process_in_wait) {
      process_in->wait_for_1ms(true);
    }
    // check if have any new processes have the same arrival time.
    check_arrival();
    // Process_out is not in the ready queue for the first half
    running->wait_for_1ms(false);
    // Process in I/O burst proceed for t_cs
    do_blocking();
    perform_add_to_ready_queue();
    time++;
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
  for (int i = start_time; i < t_cs / 2; i++) {
    // Processes in ready queue wait for t_cs
    do_waiting();
    // check if have any new processes have the same arrival time.
    check_arrival();
    // Process_in is not in the ready queue for the second half
    running->wait_for_1ms(false);
    // Process in I/O burst proceed for t_cs
    do_blocking();
    perform_add_to_ready_queue();
    time++;
  }
  std::stringstream event;
  event << "Process " << running->get_ID() << " started using the CPU for "
        << running->get_remaining_time() << " ms burst";
  print_event(event.str());
}

void schedule_algorithm::check_arrival() {

  for (auto itr = processes.begin(); itr != processes.end(); ++itr) {
    if (itr->get_arrival_time() == time) {
      prepare_add_to_ready_queue(itr);
    }
  }
}

void schedule_algorithm::do_waiting() {
  for (auto itr = ready_queue.begin(); itr != ready_queue.end(); ++itr) {
    (*itr)->wait_for_1ms(true);
  }
}

void schedule_algorithm::do_blocking() {
  for (auto itr = blocked.begin(); itr != blocked.end(); ++itr) {
    int state = (*itr)->block_for_1ms();
    // If the I/O time is end then move it to ready_queue
    if (state == 1) {
      blocked.erase(itr);
      prepare_add_to_ready_queue(*itr);
    }
  }
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
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    // block processes on I/O for 1ms
    do_blocking();
    // check if have any new processes have the same arrival time.
    check_arrival();
    // remove the running process
    // Determine whether add the running process to ready_queue
    // or blocked by its state
    if (state == 0 || state == -1) {
      if (running != processes.end()) {
        if (running->get_state() == 1)
          prepare_add_to_ready_queue(running);
        else if (running->get_state() == 0) {
          blocked.insert(running);
        } else {
          terminated.insert(running);
        }
      }
    }
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
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
  }
  pre_ready_queue.clear();
}

RR_scheduling::RR_scheduling(const std::vector<process> &p, const int t_cs,
                             const int t_slice, const bool add)
    : schedule_algorithm(p, t_cs), t_slice(t_slice), add(add) {}

void RR_scheduling::run() {
  print_event("Simulator started for RR");

  // The time the current process is running for
  int time_running = 0;
  int state = -2;
  int cs = 0;
  while (terminated.size() < processes.size()) {
    // block processes on I/O for 1ms
    do_blocking();
    // check if have any new processes have the same arrival time.
    check_arrival();
    // remove the running process
    // Determine whether add the running process to ready_queue
    // or blocked by its state
    if (state == 0 || state == -1 ||
        (time_running >= t_slice && !ready_queue.empty())) {
      if (running != processes.end()) {
        if (running->get_state() == 1) {
          std::stringstream event;
          event << "Time slice expired; process " << running->get_ID()
                << " preempted with " << running->get_remaining_time()
                << " ms to go";
          print_event(event.str());
          prepare_add_to_ready_queue(running);
        } else if (running->get_state() == 0) {
          blocked.insert(running);
        } else {
          terminated.insert(running);
        }
      }
    }
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
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
      context_switch(*(ready_queue.begin()));
      time_running = 0;
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
                                    const double lambda,const double alpha)
    :schedule_algorithm(p,t_cs), lambda(lambda),alpha(alpha){}
void SJF_scheduling::run(){
  print_event("Simulator started for SJF");
  int state=-2;
  int cs=0;
  while (terminated.size() < processes.size()){
    do_blocking();
    check_arrival();
    if (state == 0 || state == -1) {
      if (running != processes.end()) {
        if (running->get_state() == 1)
          prepare_add_to_ready_queue(running);
        else if (running->get_state() == 0) {
          blocked.insert(running);
        } else {
          terminated.insert(running);
        }
      }
    }
    perform_add_to_ready_queue();
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
  for (auto i: pre_ready_queue){
    ready_queue.push_back(i);
    std::stringstream event;
    if (i->get_arrival_time() == time) {
      i->set_estimated_remaining_time(1/lambda);//set tau0;
      event << "Process " << i->get_ID()<<" (tau "<<i->get_last_estimated_burst_time()<<"ms)" << " arrived; added to ready queue";
    } 
    else {
      i->set_estimated_remaining_time(est_tau(i->get_last_estimated_burst_time(),i->get_last_burst_time()));//
      event << "Process " << i->get_ID()
            <<" (tau "<<i->get_last_estimated_burst_time()<<"ms)"<< " completed I/O; added to ready queue";
    }
    print_event(event.str());
  }
  pre_ready_queue.clear();
  //std::sort(ready_queue.begin(), ready_queue.end(), compare_sjf);
}
int SJF_scheduling::est_tau(double tau,int t){
  double next_est=alpha*t+(1-alpha)*tau;
  return next_est;
}  

/*
//SRT
SRT_scheduling::SRT_scheduling(const std::vector<process> &p, const int t_cs,
                             const double lambda,const double alpha)
    : schedule_algorithm(p, t_cs), lambda(lambda), alpha(alpha) {}

void SRT_scheduling::run() {
  print_event("Simulator started for SRT");

  // The time the current process is running for
  int time_running = 0;
  int state = -2;
  while (terminated.size() < processes.size()) {
    // block processes on I/O for 1ms
    do_blocking();
    // check if have any new processes have the same arrival time.
    check_arrival();
    // remove the running process
    // Determine whether add the running process to ready_queue
    // or blocked by its state
    if (state == 0 || state == -1 ||
        (time_running >= t_slice && !ready_queue.empty())) {//is here to determine?
      if (running != processes.end()) {
        if (running->get_state() == 1) {
          std::stringstream event;
          event << "Time slice expired; process " << running->get_ID()
                << " preempted with " << running->get_remaining_time()
                << " ms to go";
          print_event(event.str());
          prepare_add_to_ready_queue(running);
        } 
        else if (running->get_state() == 0) {
          blocked.insert(running);
        } 
        else {
          terminated.insert(running);
        }
      }
    }
    //compare the 1st one in pre_ready_queue and the 1st one in ready_queue is the same one or not;
    std::sort(pre_ready_queue.begin(), pre_ready_queue.end(), compare_srt);//sort the pre_ready_queue first
    std::string pre_first_ID=(*(pre_ready_queue.begin()))->get_ID();//ID for resolveTie
    // loop for all the processes in the pre_ready_queue to push_back them
    // into ready queue
    perform_add_to_ready_queue();
    //get the ready_queue first ID;
    std::string ready_first_ID=(*(pre_ready_queue.begin()))->get_ID();//if the pre_ready_first is the same one. then PREEMPTE.
    // Run the running process for 1 ms. If there is no running process
    // then skip to next 1 ms.
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
    // when arrived new process have the shortest remaining time,
    if (pre_ready_first==ready_first && pre_first_ID==ready_first_ID) {//if their ID are the same one them preempt.
      context_switch(*(ready_queue.begin()));
      time_running = 0;
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
      ++time_running;
    } else {
      // no current running process
      state = -2;
    }
    time++;
  }
  print_event("Simulator ended for SRT");
}
void SRT_scheduling::perform_add_to_ready_queue() {
  for (auto i: pre_ready_queue){
    ready_queue.push_back(i);
    std::stringstream event;
    if (i->get_arrival_time() == time) {
      i->set_estimated_remaining_time(1/lambda);//set tau0;
      event << "Process " << i->get_ID() << " arrived; added to ready queue";
    } 
    else {
      i->set_estimated_remaining_time(est_tau(i->get_last_estimated_burst_time(),i->get_last_burst_time()));//
      event << "Process " << i->get_ID()
            << " completed I/O; added to ready queue";
    }
    print_event(event.str());
  }
  pre_ready_queue.clear();
  std::sort(ready_queue.begin(), ready_queue.end(), compare_srt);
}  */