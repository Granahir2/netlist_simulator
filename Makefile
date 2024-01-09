CXXFLAGS = -O3 -Wall -Wextra -std=c++20
CC = g++
OBJECTS = naive_sim.o parser.o scheduler.o type_checker.o


netlist_sim: $(OBJECTS) main.cc
	$(CC) $(CXXFLAGS) $(OBJECTS) main.cc -o netlist_sim

%.o : %.cc %.hh parser.hh

.PHONY: clean
clean :
	-rm -rf $(OBJECTS) netlist_sim logs
	-mkdir logs
