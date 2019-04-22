CXX=g++
CXXFLAGS=-Wall -Werror -std=c++11
TARGET=./main

SRC=main.cpp memory_manager.cpp

main: main.o memory_manager.o
	$(CXX) $(XCCFLAGS) -o main \
		main.o memory_manager.o
main.o: memory_manager.o main.cpp
memory_manager.o: memory_manager.cpp memory_manager.h

clean:
	rm -f *.o
	rm -f $(TARGET)
