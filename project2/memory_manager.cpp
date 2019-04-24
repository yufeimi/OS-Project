#include "memory_manager.h"

bool compare_partitions(std::pair<frame, int> a, std::pair<frame, int> b) {
  return a.first < b.first;
}

bool compare_events(std::tuple<int, process_ptr, bool> a,
                    std::tuple<int, process_ptr, bool> b) {
  int time_a = std::get<0>(a);
  int time_b = std::get<0>(b);
  if (time_a < time_b) return true;
  if (time_a > time_b) return false;
  bool inout_a = std::get<2>(a);
  bool inout_b = std::get<2>(b);
  if (!inout_a && inout_b) return true;
  if (!inout_b && inout_a) return false;
  process_ptr p_a = std::get<1>(a);
  process_ptr p_b = std::get<1>(b);
  return (p_a->ID < p_b->ID);
}

process::process(const char ID, const int size,
                 std::list<std::pair<int, int>> sequence)
    : ID(ID), size(size) {
  time_sequence = sequence;
}

process::process(const process& source) : ID(source.ID), size(source.size) {
  time_sequence = source.time_sequence;
}

void process::delay(const int time) {
  for (auto& i : time_sequence) {
    i.first += time;
  }
}

void process::operator=(process p) { this->time_sequence = p.time_sequence; }

memory_manager::memory_manager(std::vector<process>& processes,
                               const int m_size, const int line_length,
                               const int t_memmove)
    : processes(processes),
      time(0),
      memory_size(m_size),
      line_length(line_length),
      t_memmove(t_memmove) {
  this->reset();
}

void memory_manager::reset() {
  time = 0;
  allocations.clear();
  partitions.clear();
  partitions.push_front({0, memory_size});
  memory.clear();
  memory.resize(memory_size, '.');
  construct_time_table();
}

void memory_manager::construct_time_table() {
  time_table.clear();
  for (auto p = processes.begin(); p != processes.end(); ++p) {
    for (auto i : p->time_sequence) {
      time_table.push_back(std::make_tuple(i.first, p, true));
      time_table.push_back(std::make_tuple(i.first + i.second, p, false));
    }
  }
  time_table.sort(compare_events);
}

void memory_manager::add(frame location, process_ptr p, int allocation_size) {
  // Want to check first if we can find the location in spare partitions.
  // If we find it then we shrink the size of the partition
  bool spare_check = false;
  for (auto i = partitions.begin(); i != partitions.end(); ++i) {
    if (location == i->first) {
      spare_check = true;
      // The partition size must be no less than allocation size.
      assert(i->second >= allocation_size);
      // Shrink the partition by moving it forward
      i->second -= allocation_size;
      if (i->second == 0)
        i = partitions.erase(i);
      else
        i->first += allocation_size;
      break;
    }
  }
  spare_check = true;
  assert(spare_check);
  // Put the allocation in allocations
  allocations.push_front(std::make_tuple(location, p, allocation_size));
  for (frame i = location; i < location + allocation_size; ++i) {
    memory[i] = p->ID;
  }
}

allocation_ptr memory_manager::remove(allocation_ptr to_be_removed) {
  frame start_location = std::get<0>(*to_be_removed);
  int psize = std::get<2>(*to_be_removed);
  // Check adjacencies. If there is an adjacent spare
  // partition, expand that partition (or connect two
  // partitions!). If not, create a new partition.
  frame end_location = start_location;
  end_location += psize - 1;
  bool adjacent_partition = false;
  for (auto& i : partitions) {
    // If there is a partition before
    if (i.first + i.second - 1 == start_location) {
      i.second += psize;
      adjacent_partition = true;
      break;
    }
    // If there is a parition after
    else if (i.first == (end_location + 1)) {
      i.second += psize;
      i.first -= psize;
      adjacent_partition = true;
      break;
    }
  }
  // If no adjacent partition then create a new partition
  if (!adjacent_partition) {
    partitions.push_back({start_location, psize});
  }
  for (frame i = start_location; i < end_location + 1; ++i) {
    memory[i] = '.';
  }
  auto return_itr = allocations.erase(to_be_removed);
  // Check connected partitions and union them
  partitions.sort(compare_partitions);
  for (auto i = partitions.begin(); i != partitions.end(); ++i) {
    auto tmp = i;
    if (i->first + i->second == (++tmp)->first) {
      i->second += tmp->second;
      partitions.erase(tmp);
    }
  }
  return return_itr;
}

int memory_manager::defragmentation() {
  int total_time = 0;
  bool complete = false;
  std::list<char> moved_allocations;
  while (!complete) {
    partitions.sort(compare_partitions);
    for (auto& p : partitions) {
      if (p.first + p.second < (size_t)memory_size &&
          memory[p.first + p.second] != '.') {
        // Find the allocation after it
        for (auto a = allocations.begin(); a != allocations.end(); ++a) {
          size_t location = std::get<0>(*a);
          process_ptr process = std::get<1>(*a);
          size_t psize = std::get<2>(*a);
          if (location == p.first + p.second) {
            remove(a);
            add(p.first, process, psize);
            moved_allocations.push_back(process->ID);
            total_time += psize * t_memmove;
            break;
          }
        }
        break;
      }
      if (p.first + p.second == (size_t)memory_size) complete = true;
    }
  }
  std::cout << "time " << time + total_time
            << "ms: Defragmentation complete (moved " << total_time / t_memmove
            << " frames: ";
  for (auto i : moved_allocations) {
    std::cout << i;
    if (i != moved_allocations.back()) std::cout << ", ";
  }

  std::cout << ")" << std::endl;
  return total_time;
}

void memory_manager::print_memory() {
  for (int i = 0; i < line_length; ++i) {
    std::cout << "=";
  }
  std::cout << std::endl;
  for (unsigned int i = 0; i < memory.size(); ++i) {
    std::cout << memory[i];
    if ((i + 1) % line_length == 0 && i + 1 != memory.size())
      std::cout << std::endl;
  }
  std::cout << std::endl;
  for (int i = 0; i < line_length; ++i) {
    std::cout << "=";
  }
  std::cout << std::endl;
}

void memory_manager::run(algorithm algo) {
  std::cout << "time 0ms: Simulator started ";
  switch (algo) {
    case first_fit:
      std::cout << "(Contiguous -- First-Fit)\n";
      break;
    case next_fit:
      std::cout << "(Contiguous -- Next-Fit)\n";
      break;
    case best_fit:
      std::cout << "(Contiguous -- Best-Fit)\n";
      break;
    case non_con:
      std::cout << "(Non-Contiguous)\n";
      break;
    default:
      std::cout << "Unknown!\n";
  }
  // Only for next-fit. Store the location of last memory allocation
  frame last_allocation_end = 0;
  // Fetch the next event;
  while (not time_table.empty()) {
    event next_event = std::move(time_table.front());
    time_table.pop_front();
    // Pull the informations from the event
    int event_time = std::get<0>(next_event);
    process_ptr event_process = std::get<1>(next_event);
    bool event_inout = std::get<2>(next_event);
    // Cut the time between now and the event
    assert(event_time >= time);
    // If the event is to place an allocation in
    if (event_inout) {
      time = event_time;
      std::cout << "time " << time << "ms: Process " << event_process->ID
                << " arrived (requires " << event_process->size << " frames)\n";
      // Determine where to palce
      if (algo == first_fit) {
        partitions.sort(compare_partitions);
        // Look for the first available partition
        bool available = false;
        int total_free_space = 0;
        for (auto i : partitions) {
          // Compare partition size with the expected allocation size
          if (i.second >= event_process->size) {
            add(i.first, event_process, event_process->size);
            available = true;
            break;
          }
          total_free_space += i.second;
        }
        if (available) {
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else if (total_free_space >= event_process->size) {
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- starting defragmentation\n";
          // Do defragmentation
          int time_defrag = defragmentation();
          // Delay all the future events
          for (auto& i : time_table) std::get<0>(i) += time_defrag;
          time += time_defrag;
          // There should be only one partition
          assert(partitions.size() == 1);
          add(partitions.front().first, event_process, event_process->size);
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else {  // No space for this memory
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- skipped!\n";
        }
      } else if (algo == next_fit) {
        partitions.sort(compare_partitions);
        // Look for the first available partition
        bool available = false;
        for (auto i : partitions) {
          if (i.first + i.second < last_allocation_end + event_process->size)
            continue;
          // Compare partition size with the expected allocation size
          if (i.second >= event_process->size) {
            frame frame_to_be_allocated =
                (i.first > last_allocation_end ? i.first : last_allocation_end);
            last_allocation_end = frame_to_be_allocated + event_process->size;
            add(frame_to_be_allocated, event_process, event_process->size);
            available = true;
            break;
          }
        }
        int total_free_space = 0;
        if (!available) {
          for (auto i : partitions) {
            // Compare partition size with the expected allocation size
            if (i.second >= event_process->size) {
              last_allocation_end = i.first + event_process->size;
              add(i.first, event_process, event_process->size);
              available = true;
              break;
            }
            total_free_space += i.second;
          }
        }
        if (available) {
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else if (total_free_space >= event_process->size) {
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- starting defragmentation\n";
          // Do defragmentation
          int time_defrag = defragmentation();
          // Delay all the future events
          for (auto& i : time_table) std::get<0>(i) += time_defrag;
          time += time_defrag;
          // There should be only one partition
          assert(partitions.size() == 1);
          add(partitions.front().first, event_process, event_process->size);
          last_allocation_end = partitions.front().first;
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else {  // No space for this memory
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- skipped!\n";
        }
      } else if (algo == best_fit) {
        partitions.sort(compare_partitions);
        bool available = false;
        int total_free_space = 0;
        std::pair<frame, int> least_available_partition = {0, memory_size + 1};
        for (auto i : partitions) {
          // Compare partition size with the expected allocation size
          if (i.second >= event_process->size) {
            available = true;
            if (i.second < least_available_partition.second)
              least_available_partition = i;
          }
          total_free_space += i.second;
        }
        if (available) {
          add(least_available_partition.first, event_process,
              event_process->size);
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else if (total_free_space >= event_process->size) {
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- starting defragmentation\n";
          // Do defragmentation
          int time_defrag = defragmentation();
          // Delay all the future events
          for (auto& i : time_table) std::get<0>(i) += time_defrag;
          time += time_defrag;
          // There should be only one partition
          assert(partitions.size() == 1);
          add(partitions.front().first, event_process, event_process->size);
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else {  // No space for this memory
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- skipped!\n";
        }
      } else if (algo == non_con) {
        partitions.sort(compare_partitions);
        int total_free_space = 0;
        for (auto i : partitions) {
          total_free_space += i.second;
        }
        if (total_free_space >= event_process->size) {
          // Mark the partitions to be allocated
          std::list<std::pair<int, int>> marked_partitions;
          int space_to_be_allocated = event_process->size;
          for (auto i : partitions) {
            int psize =
                (space_to_be_allocated > i.second ? i.second
                                                  : space_to_be_allocated);
            marked_partitions.push_back({i.first, psize});
            space_to_be_allocated -= psize;
            if (space_to_be_allocated == 0) break;
          }
          for (auto i : marked_partitions) {
            add(i.first, event_process, i.second);
          }
          std::cout << "time " << time << "ms: Placed process "
                    << event_process->ID << ":\n";
          print_memory();
        } else {  // No space for this memory
          std::cout << "time " << time << "ms: Cannot place process "
                    << event_process->ID << " -- skipped!\n";
        }
      } else {
        std::cerr << "Unknown algorithm!\n";
        return;
      }
    }
    // If we want to remove a process
    else {
      bool found_process = false;
      for (auto itr = allocations.begin(); itr != allocations.end(); ++itr) {
        if (std::get<1>(*itr)->ID == event_process->ID) {
          while (std::get<1>(*itr)->ID == event_process->ID) {
            itr = remove(itr);
            if (itr == allocations.end()) break;
          }
          found_process = true;
        }
      }
      if (found_process) {
        time = event_time;
        std::cout << "time " << time << "ms: Process " << event_process->ID
                  << " removed:\n";
        print_memory();
      }
    }
  }
  std::cout << "time " << time << "ms: Simulator ended ";
  switch (algo) {
    case first_fit:
      std::cout << "(Contiguous -- First-Fit)\n";
      break;
    case next_fit:
      std::cout << "(Contiguous -- Next-Fit)\n";
      break;
    case best_fit:
      std::cout << "(Contiguous -- Best-Fit)\n";
      break;
    case non_con:
      std::cout << "(Non-Contiguous)\n";
      break;
    default:
      std::cout << "Unknown!\n";
  }
}
