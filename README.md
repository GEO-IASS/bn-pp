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
-b  solve query using bayes-ball
-h	display help information
-v	verbose
```

To inspect the markov assumptions of asia model

```
$ ./bn ../models/asia.uai -v <../models/asia.markov.query >markov.result.txt
```

To check the local markov independencies of asia model

```
$ ./bn ../models/asia.uai <../models/asia.ind
```

To check variable dependencies of asia model

```
./bn ../models/asia.uai <../models/asia.not.ind
```

To execute queries or check (in)dependencies on the prompt
```
$ ./bn ../models/asia.uai

>> Query prompt:

? query 1 | 0, 2
P(1|0,2) =
Factor(width:2, size:4, partition:2)
1 0
0 0 : 0.99000
0 1 : 0.95000
1 0 : 0.01000
1 1 : 0.05000
>> Executed in 0.24763ms.

? query 2, 3 ,4
P(2,3,4) =
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
>> Executed in 0.24945ms.

? ind 7,1|4,5
true

? ind 4,1|6
false

? quit
```

## Input

The input format is the uai model specification for BAYES network [UAI 2014 Inference Competition](http://www.hlt.utdallas.edu/~vgogate/uai14-competition/).
