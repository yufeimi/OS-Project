#include <fstream>
#include <iostream>
#include <vector>
#include "memory_manager.h"

std::vector<process> parse_input(std::ifstream);

int main(int argc, char const *argv[]) {
  std::list<std::pair<int, int>> sequence;
  sequence.push_front({500, 200});
  process p1('A', 5, sequence);
  process p2('B', 20, sequence);
  std::vector<process> processes({p1, p2});
  memory_manager m(processes, 100, 20, 1);
  m.run(first_fit);
  return 0;
}

std::vector<process> parse_input(std::ifstream inputfile) {
  std::vector<process> processes;

  return processes;
};
