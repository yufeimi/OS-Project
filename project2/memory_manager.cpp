#include "memory_manager.h"

bool compare_partitions(std::pair<frame, int> a, std::pair<frame, int> b) {
  return a < b;
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

memory_manager::memory_manager(std::vector<process>& processes_at_start,
                               const int m_size, const int line_length,
                               const int t_memmove)
    : processes_at_start(processes),
      time(0),
      memory_size(m_size),
      line_length(line_length),
      t_memmove(t_memmove) {
  processes = processes_at_start;
  this->reset();
}

void memory_manager::reset() {
  processes.clear();
  processes = processes_at_start;
  time = 0;
  allocations.clear();
  partitions.clear();
  partitions.push_front({0, memory_size});
  memory.clear();
  memory.resize(memory_size, '.');
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

int memory_manager::defragmentation() {
  int total_time = 0;
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
  auto itr = processes.begin();
  add(0, itr, itr->size);
  ++itr;
  add(5, itr, itr->size);
  print_memory();
  remove(++allocations.begin());
  print_memory();
}
