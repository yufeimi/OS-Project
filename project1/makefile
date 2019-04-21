CXX=g++
CXXFLAGS=-Wall -Werror -std=c++17
TARGET=./main

SRC=main.cpp process.cpp schedule_algorithm.cpp

main: main.o process.o schedule_algorithm.o
	$(CXX) $(XCCFLAGS) -o main \
		main.o process.o schedule_algorithm.o
main.o: process.o schedule_algorithm.o main.cpp
process.o: process.cpp process.h
schedule_algorithm.o: schedule_algorithm.cpp schedule_algorithm.h

clean:
	rm -f *.o
	rm -f $(TARGET)
