# Raft on Contiki
 Implementation of the Raft Consensus Algorithm for the Contiki Operating System on TI's CC2650 Sensortag
 * [Raft Consensus Algorithm](https://raft.github.io/)
 * [Contiki Operating System](http://www.contiki-os.org/)
 * [Contiki Source](https://github.com/contiki-os/contiki)
 * [TI CC2650 Sensortag] (http://processors.wiki.ti.com/index.php/SensorTag2015)
## Modifying the Makefile
 1. Rename Makefile_sample to Makefile
 2. Modify the paths from the project folder to your instances of the arm-gcc compiler and the contiki source<br>
 ``` COMPILERPATH = /path/to/arm/gcc/compiler/bin ```<br>
 ``` CONTIKI = path/to/contiki ```
## Modifying the Raft Settings
 Edit the macros in raft.h<br>
 ```c
 #define UDP_PORT 1234 //UDP Broadcast Port for messaging
 #define MIN_TIMEOUT 3 //minimum value for timeout
 #define MAX_TIMEOUT 7 //maximum value for timeout
 #define LEADER_SEND_INTERVAL (MIN_TIMEOUT / 2)
 #define TOTAL_NODES 4 //total number of nodes in network
 ```
## Compiling the Raft Node Source and Make Options
 ``` make all ``` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Clean directory, compile Contiki and Raft Node source<br>
 ``` make raft_node ``` &nbsp;Compile Raft Node source<br>
 ``` make clean ``` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Clean directory
