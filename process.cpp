#include "process.h"

process::process() : arrival_time(0), ID('A') {}

process::process(const process &p) : arrival_time(p.arrival_time), ID(p.ID) {
  this->time_sequence = p.time_sequence;
  this->reset();
}

process::process(const int t, char id, const std::vector<int> &time_sequence)
    : arrival_time(t), ID(id), time_sequence(time_sequence) {
  // Size must be odd. Since first and last bursts are CPU
  assert(time_sequence.size() % 2);
  this->reset();
}

const int process::get_remaining_CPU_bursts() const {
  int bursts = 0;
  int tmp_time = current_time;
  for (unsigned int i = 0; i < time_sequence.size(); ++i) {
    if (tmp_time > time_sequence[i])
      tmp_time -= time_sequence[i];
    else {
      bursts = (time_sequence.size() - i) / 2;
      break;
    }
    if (i == time_sequence.size() - 1) {
      break;
    }
  }
  return bursts;
}

const int process::preempted() const {
  return (current_time && query_state(current_time - 1) && state);
}

const int process::get_last_burst_time() const {
  int bursted = time_sequence.size() / 2 - get_remaining_CPU_bursts();
  return time_sequence[2 * bursted];
}

void process::set_estimated_remaining_time(const int t) {
  estimated_remaining_time = t;
  last_estimated_burst_time = t;
}

const int process::run_for_1ms() {
  assert(state == 1);
  const int next = this->proceed();
  ++turnaround_time;
  if (next) {
    --remaining_time;
    --estimated_remaining_time;
  }
  return next;
}

const int process::wait_for_1ms(bool increase_wait_time = true) {
  if (increase_wait_time)
    ++wait_time;
  // Only increase turnaround time after first running
  if (query_state(current_time - 1) == 1)
    ++turnaround_time;
  return state;
}

const int process::block_for_1ms() {
  assert(state == 0);
  --remaining_time;
  const int next = this->proceed();
  return next;
}

void process::print() {
  std::cout << "Process " << ID << " [NEW] (arrival time " << arrival_time
            << " ms) " << time_sequence.size() / 2 + 1 << " CPU bursts\n";
  for (unsigned int i = 0; i < time_sequence.size(); ++i) {
    std::cout << "--> CPU burst " << time_sequence[i] << " ms";
    ++i;
    if (i < time_sequence.size())
      std::cout << " --> I/O burst " << time_sequence[i] << " ms";
    std::cout << std::endl;
  }
}

void process::print_overview() {
  std::string plural = time_sequence.size() > 1 ? " bursts" : " burst";
  std::cout << "Process " << ID << " [NEW] (arrival time " << arrival_time
            << " ms) " << time_sequence.size() / 2 + 1 << " CPU" << plural
            << std::endl;
}

void process::reset() {
  wait_time = 0;
  turnaround_time = 0;
  current_time = 0;
  state = 1;
  remaining_time = 0;
  total_time = 0;
  estimated_remaining_time = 0;
  last_estimated_burst_time = 0;
  for (unsigned int i = 0; i < time_sequence.size(); ++i) {
    total_time += time_sequence[i];
  }
  compute_remaining_time();
}

const int process::proceed() {
  assert(current_time < total_time);
  const int current = query_state(current_time);
  const int next = query_state(++current_time);
  state = next;
  if (next == -1) {
    return -1;
  }
  // If switch between CPU burst and I/O burst
  if (current != next) {
    if (next) {
      // Reset turnaround time
      turnaround_time = 0;
      wait_time = 0;
    }
    // Calculate remaining time
    remaining_time = 0;
    this->compute_remaining_time();
  }
  return next;
}

const int process::query_state(int time) const {
  int state = 0;
  for (unsigned int i = 0; i < time_sequence.size(); ++i) {
    if (time >= time_sequence[i])
      time -= time_sequence[i];
    else {
      state = 1 - (i % 2);
      break;
    }
    if (i == time_sequence.size() - 1) {
      state = -1;
      break;
    }
  }
  return state;
}

void process::compute_remaining_time() {
  int tmp_time = current_time;
  for (unsigned int i = 0; i < time_sequence.size(); ++i) {
    if (tmp_time >= time_sequence[i]) {
      tmp_time -= time_sequence[i];
    } else {
      remaining_time = time_sequence[i] - tmp_time;
      break;
    }
  }
}
