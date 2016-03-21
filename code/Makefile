CC=g++
CXXFLAGS=-Wall -Wextra -ansi -pedantic -std=c++11 -g

OBJ=variable.o domain.o factor.o model.o io.o

all: bn mn

bn: $(OBJ) bn.o
	$(CC) $^ -o $@

bn.o: bn.cpp
	$(CC) $(CXXFLAGS) -c $<

mn: $(OBJ) mn.o
	$(CC) $^ -o $@

mn.o: mn.cpp
	$(CC) $(CXXFLAGS) -c $<

model.o: model.cpp model.hh
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
	rm -rvf .DS_Store *~ bn bn.dSYM/ mn mn.dSYM/ *.o
