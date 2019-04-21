#include <assert.h>
#include <iostream>
#include <list>
#include <tuple>
#include <vector>

class process;
class memory_manager;

typedef unsigned int frame;
typedef std::vector<process>::iterator process_ptr;
typedef std::list<std::tuple<frame, process_ptr, int>>::iterator allocation_ptr;
typedef std::list<std::pair<frame, int>>::iterator partition_ptr;

enum algorithm { first_fit, next_fit, best_fit, non_con };

struct process {
  // Constructor.
  process(const char, const int, std::list<std::pair<int, int>>);
  // Copy constructor
  process(const process&);
  // Operator overload
  void operator=(process p);
  // Delay all the process by an amount of time.
  void delay(const int);
  // Process ID. A, B, C, etc.
  const char ID;
  // Memory allocation size.
  const int size;
  // Stores a list of arrival_time/duration.
  std::list<std::pair<int, int>> time_sequence;
};

class memory_manager {
 public:
  // Constructor.
  memory_manager(std::vector<process>&, const int m_size, const int line_length,
                 const int t_memmove);
  // Reset all the processes to the start status. Call it before running!
  void reset();
  // Execute memory manager. Execution algorithm is one of below:
  // first_fit, next_fit, best_fit, non_con
  void run(algorithm algo);

 private:
  // Add a memory allocation to the physical memory
  void add(frame, process_ptr, int);
  // Remove a memory allocation from the physical memory
  void remove(allocation_ptr);
  // Defragmentation. Returns total time for this operation in ms.
  int defragmentation();
  // print memory.
  void print_memory();
  // A vector storing all the processes.
  std::vector<process> processes;
  // A vector stroing all the processes at the start status (i.e., no execution
  // performed).
  const std::vector<process> processes_at_start;
  // Physical memory "A-Z" for allocated memory frame ID. "." for spare memory
  // frame
  std::vector<char> memory;
  // Physical time.
  int time;
  // Physical memory size in unit of frames
  const int memory_size;
  // Line length for print
  const int line_length;
  // time required for moving ONE memory frame
  const int t_memmove;
  // list indicating where processes are allist<std::tuple<frame, process_ptr,
  // int>> allocation_ptr;
  // located of tuple < location, process, size> NOTE: for contiguous algorithm,
  // the size is always equal to process memory size.
  std::list<std::tuple<frame, process_ptr, int>> allocations;
  // List indicating spare memory partitions of pair<location, size>
  std::list<std::pair<frame, int>> partitions;
};
