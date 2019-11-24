How to Run?

The project contains 4 CPP files berkley.cpp, causal_ordering.cpp and non_causal_ordering.cpp and locking.cpp

All these files can be compiled in one go just by using the target, make compile.
The target make clean will clean all the object files.

Depending upon the program output requirements each object file runs differently. Following are the steps to execute each of these files,

1. berkley.cpp
For the program simplicity I have assumed 4 default ports which are 54002 54003 54004 and 54001.

Launch slave process 2 on port 54002
./berkley 54002 2

Launch slave process 3 on port 54003
./berkley 54003 3

Launch slave process 4 on port 54004
./berkley 54004 4


Finally launch master process 1 on port 54001
./berkley 54001 1


2. causal_ordering.cpp

Launch multicast process 1
./causal_ordering 1

Launch multicast process 2
./causal_ordering 2

Launch multicast process 3
./causal_ordering 3

No need to pass port no in the arguments here


3. non_causal_ordering.cpp

Launch multicast process 1
./ non_causal_ordering 1

Launch multicast process 2
./ non_causal_ordering 2

Launch multicast process 3
./ non_causal_ordering 3

No need to pass port no in the arguments here




4. locking.cpp

Launch slave process 2 on port 54002
./locking 54002 2

Launch slave process 3 on port 54003
./locking 54003 3

Launch slave process 4 on port 54004
./locking 54004 4


Finally launch coordinator process 1 on port 54001
./coordinator 54001 1


If any there is some issue in executing anything please feel free to call me: 443 938 8830
