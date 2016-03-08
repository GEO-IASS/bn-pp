# bn-pp

BN++ Data Structures and Algorithms in C++ for Bayesian Networks.

## Compilation

```
$ make clean && make
```

## Usage

```
$ ./bn
usage: ./bn /path/to/model.uai [OPTIONS]

OPTIONS:
-h	display help information
-v	verbose
```

To inspect the markov assumptions of asia model

```
$ ./bn ../models/asia.uai -v <../models/asia.markov.query >markov.result.txt
```

To execute some queries on the prompt
```
$ ./bn ../models/asia.uai

>> Query prompt:

? query 1 | 0, 2
Factor(width:3, size:8, partition:4.00000)
0 1 2
0 0 0 : 0.99000
0 0 1 : 0.99000
0 1 0 : 0.01000
0 1 1 : 0.01000
1 0 0 : 0.95000
1 0 1 : 0.95000
1 1 0 : 0.05000
1 1 1 : 0.05000

? query 2, 3 ,4
Factor(width:3, size:8, partition:1.00000)
2 3 4
0 0 0 : 0.34650
0 0 1 : 0.14850
0 1 0 : 0.00350
0 1 1 : 0.00150
1 0 0 : 0.18000
1 0 1 : 0.27000
1 1 0 : 0.02000
1 1 1 : 0.03000

? quit
```

## Input

The input format is the uai model specification for BAYES network [UAI 2014 Inference Competition](http://www.hlt.utdallas.edu/~vgogate/uai14-competition/).
