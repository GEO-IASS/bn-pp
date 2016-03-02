CC=g++
CXXFLAGS=-Wall -Wextra -ansi -pedantic -std=c++11

all: bn

bn: main.o
	$(CC) $^ -o $@

main.o: main.cpp
	$(CC) $(CXXFLAGS) -c $^

.PHONY: clean
clean:
	rm -rvf .DS_Store *~ bn *.o
