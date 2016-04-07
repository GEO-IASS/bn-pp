CC=g++
CXXFLAGS=-Wall -Wextra -ansi -pedantic -std=c++11 -O2

OBJ=utils.o graph.o variable.o domain.o factor.o model.o io.o

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

graph.o: graph.cpp graph.hh
	$(CC) $(CXXFLAGS) -c $<

utils.o: utils.cpp utils.hh
	$(CC) $(CXXFLAGS) -c $<

.PHONY: clean check check-bn check-mn
clean:
	rm -rvf .DS_Store *~ bn bn.dSYM/ mn mn.dSYM/ *.o

check: check-bn check-mn

check-bn: bn
	valgrind --leak-check=full ./bn ../models/asia.uai -v -ve -bb <../models/asia.markov.query ; \
	valgrind --leak-check=full ./bn ../models/asia.uai ../models/asia.uai.evid -pr ; \
	valgrind --leak-check=full ./bn ../models/asia.uai ../models/asia.uai.evid -mar ; \
	valgrind --leak-check=full ./bn ../models/asia.uai <../models/asia.ind  ; \
	valgrind --leak-check=full ./bn ../models/asia.uai <../models/asia.not.ind

check-mn: mn
	valgrind --leak-check=full ./mn ../models/grid3x3.uai ../models/grid3x3-PR.uai.evid <../models/grid3x3-PR.uai.query ; \
	valgrind --leak-check=full ./mn ../models/grid3x3.uai ../models/grid3x3-MAR.uai.evid <../models/grid3x3-MAR.uai.query
