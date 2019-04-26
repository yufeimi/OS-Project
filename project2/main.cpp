#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "memory_manager.h"

std::vector<process> parse_input(std::ifstream &);

int main(int argc, char const *argv[]) {
  if (argc < 5) std::cerr << "Wrong usage!\n";
  int n_frames_line = atoi(argv[1]);
  int n_frames = atoi(argv[2]);
  int time_memmove = atoi(argv[4]);
  std::ifstream input;
  input.open(argv[3]);
  std::vector<process> &&processes = parse_input(input);
  input.close();
  memory_manager m(processes, n_frames, n_frames_line, time_memmove);
  m.run(first_fit);
  m.reset();
  std::cout << std::endl;
  m.run(next_fit);
  m.reset();
  std::cout << std::endl;
  m.run(best_fit);
  m.reset();
  std::cout << std::endl;
  m.run(non_con);
  return 0;
}

std::vector<process> parse_input(std::ifstream &inputfile) {
  std::vector<process> processes;
  std::string line, token, subline;
  while (getline(inputfile, line)) {
    // Get ID
    if (!line.empty() && isalpha(line[0])) {
      std::stringstream parsed(line);
      // Get ID and size;
      char ID;
      int psize;
      parsed >> ID >> psize;
      if (ID == '#') continue;
      parsed.ignore(10, ' ');
      // Get time sequence
      std::list<std::pair<int, int>> sequence;
      while (getline(parsed, subline, ' ')) {
        bool found_comment = false;
        // Get rid of comments
        size_t pos = subline.find('#');
        if (pos != std::string::npos) {
          subline = subline.substr(0, pos);
          found_comment = true;
        }
        pos = subline.find('/');
        if (pos == std::string::npos) break;
        token = subline.substr(0, pos);
        int arrival_time = std::stoi(token);
        subline.erase(0, pos + 1);
        pos = subline.find(' ');
        if (pos == std::string::npos) pos = subline.length() - 1;
        token = subline.substr(0, pos);
        int duration = std::stoi(subline);
        subline.erase(0, pos + 1);
        sequence.push_back({arrival_time, duration});
        if (found_comment) break;
      }
      process p(ID, psize, sequence);
      processes.push_back(p);
    }
  }
  return processes;
};
