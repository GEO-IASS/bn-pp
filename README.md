# bn-pp

BN++ Data Structures and Algorithms in C++ for Bayesian Networks.

## Compilation

```
$ make clean && make
```

## Usage

### Bayes nets

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
$ ./bn ../models/asia.uai <../models/asia.not.ind
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

### Markov nets

```
$ ./mn
usage: ./mn /path/to/model.uai /path/to/evidence.evid [OPTIONS]

OPTIONS:
-h	display help information
-v	verbose
```

To compute the partition function of a Markov network given evidence
```
$ ./mn ../models/grid3x3.uai ../models/grid3x3-PR.uai.evid

>> Query prompt:
? PR
partition = 14.8899

>> Executed in 22.5963ms.
```

To compute the all the marginals of a Markov network given evidence
```
>> Query prompt:
? MAR
>> Marginals:
Factor(width:0, size:1, partition:1)

: 1.00000

Factor(width:1, size:2, partition:1.00000)
1
0 : 0.00011
1 : 0.99989

Factor(width:1, size:2, partition:1.00000)
2
0 : 0.17139
1 : 0.82861

Factor(width:0, size:1, partition:1.00000)

: 1.00000

Factor(width:0, size:1, partition:1.00000)

: 1.00000

Factor(width:1, size:2, partition:1.00000)
5
0 : 0.42988
1 : 0.57012

Factor(width:1, size:2, partition:1.00000)
6
0 : 0.99122
1 : 0.00878

Factor(width:1, size:2, partition:1.00000)
7
0 : 0.99995
1 : 0.00005

Factor(width:1, size:2, partition:1.00000)
8
0 : 0.57011
1 : 0.42989

>> Executed in 22.15187ms.
```

## Input

The input format is the uai model specification for BAYES network [UAI 2014 Inference Competition](http://www.hlt.utdallas.edu/~vgogate/uai14-competition/).
