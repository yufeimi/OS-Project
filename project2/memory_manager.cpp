#include "memory_manager.h"

bool compare_partitions(std::pair<frame, int> a, std::pair<frame, int> b) {
  return a < b;
}

process::process(const char ID, const int size,
                 std::list<std::pair<int, int>>& sequence)
    : ID(ID), size(size) {
  sequence = time_sequence;
}

process::process(const process& source) : ID(source.ID), size(source.size) {
  time_sequence = source.time_sequence;
}

void process::delay(const int time) {
  for (auto& i : time_sequence) {
    i.first += time;
  }
}

process process::operator=(process p) {
  process new_process(p);
  return new_process;
}

memory_manager::memory_manager(std::vector<process>& processes_at_start,
                               const int m_size, const int t_memmove)
    : processes_at_start(processes),
      time(0),
      memory_size(m_size),
      t_memmove(t_memmove) {
  processes = processes_at_start;
  memory.resize(memory_size, '.');
}

void memory_manager::reset() {
  processes = processes_at_start;
  time = 0;
  allocations.clear();
  partitions.clear();
  memory.clear();
  memory.resize(memory_size, '.');
}

void memory_manager::add(frame location, process_ptr p, int allocation_size) {
  // Want to check first if we can find the location in spare partitions.
  // If we find it then we shrink the size of the partition
  bool spare_check;
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
  assert(spare_check);
  // Put the allocation in allocations
  allocations.push_front(std::make_tuple(location, p, allocation_size));
  for (frame i = location; i < location + allocation_size; ++i) {
    memory[i] = p->ID;
  }
}

void memory_manager::remove(allocation_ptr to_be_removed) {
  frame start_location = std::get<0>(*to_be_removed);
  int psize = std::get<2>(*to_be_removed);
  // Check adjacencies. If there is an adjacent spare
  // partition, expand that partition (or connect two
  // partitions!). If not, create a new partition.
  frame end_location = start_location;
  end_location += psize - 1;
  for (auto& i : partitions) {
    // If there is a partition before
    if (i.first + i.second - 1 == start_location) {
      i.second += psize;
      break;
    }
    // If there is a parition after
    else if (i.first == (end_location + 1)) {
      i.second += psize;
      i.first -= psize;
      break;
    }
  }
  for (frame i = start_location; i < end_location + 1; ++i) {
    memory[i] = '.';
  }
  allocations.erase(to_be_removed);
  // Check connected partitions and union them
  partitions.sort(compare_partitions);
  for (auto i = partitions.begin(); i != partitions.end(); ++i) {
    auto tmp = i;
    if (i->first + i->second == (++tmp)->first) {
      partitions.erase(tmp);
    }
  }
}

void memory_manager::run(algorithm algo) {}
