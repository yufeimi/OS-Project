CXX=g++
CXXFLAGS=-Wall -Werror
TARGET=./main

SRC=main.cpp process.cpp schedule_algorithm.cpp

main: main.o process.o schedule_algorithm.o
	$(CXX) $(XCCFLAGS) -o main \
		main.o process.o schedule_algorithm.o
main.o: process.o schedule_algorithm.o main.cpp
process.o: process.cpp
schedule_algorithm.o: schedule_algorithm.cpp

clean:
	rm -f *.o
	rm -f $(TARGET)
