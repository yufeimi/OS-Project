#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "memory_manager.h"

std::vector<process> parse_input(std::ifstream &);

int main(int argc, char const *argv[]) {
  std::ifstream input;
  input.open("p2-input01.txt");
  auto processes = parse_input(input);
  input.close();
  memory_manager m(processes, 100, 20, 1);
  m.run(first_fit);
  return 0;
}

std::vector<process> parse_input(std::ifstream &inputfile) {
  std::vector<process> processes;
  std::string line;
  getline(inputfile, line, ' ');
  while (line[0] != EOF) {
    // Get ID
    if (isalpha(line[0])) {
      char ID = line[0];
      // Get size
      getline(inputfile, line, ' ');
      int psize = std::stoi(line);
      // Get time sequence
      std::list<std::pair<int, int>> sequence;
      getline(inputfile, line);
      while (!line.empty()) {
        std::string token;
        size_t pos = line.find('/');
        if (pos == std::string::npos) break;
        token = line.substr(0, pos);
        int arrival_time = std::stoi(token);
        line.erase(0, pos + 1);
        pos = line.find(' ');
        if (pos == std::string::npos) pos = line.length() - 1;
        token = line.substr(0, pos);
        int duration = std::stoi(line);
        line.erase(0, pos + 1);
        sequence.push_back({arrival_time, duration});
      }
      process p(ID, psize, sequence);
      processes.push_back(p);
      getline(inputfile, line, ' ');
    } else
      break;
  }
  return processes;
};
