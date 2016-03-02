CC=g++
CXXFLAGS=-Wall -Wextra -ansi -pedantic -std=c++11

OBJ=variable.o domain.o factor.o io.o main.o

all: bn

bn: $(OBJ)
	$(CC) $^ -o $@

main.o: main.cpp
	$(CC) $(CXXFLAGS) -c $<

io.o: io.cpp io.hh
	$(CC) $(CXXFLAGS) -c $<

factor.o: factor.cpp factor.hh
	$(CC) $(CXXFLAGS) -c $<

domain.o: domain.cpp domain.hh
	$(CC) $(CXXFLAGS) -c $<

variable.o: variable.cpp variable.hh
	$(CC) $(CXXFLAGS) -c $<

.PHONY: clean
clean:
	rm -rvf .DS_Store *~ bn bn.dSYM/ *.o
