# CoDES
CoDES is a coroutine-based network simulation programming paradigm. This repository is a specific implementation of CoDES based on ns-3, and it includes CoDES-based MPI, CoDES-based HPCC, and CoDES-based RIP. CoDES addresses classic software engineering issues caused by callbacks, enabling developers to program in sequential logic, thereby improving development efficiency.

## Quick Start
A simple example to show how to use CoDES-based MPI under the `MPI` branch.

### Building up and Configuring NS-3
1. Build up NS-3 environment.
``` shell
cmake -DCMAKE_BUILD_TYPE=Debug -DNS3_MPI=ON -G 'CodeBlocks - Unix Makefiles' -S . -B build
```

### Preparing Trace Files
1. The HPCG trace files are already in `scrach/rn/traces`, and the file format is as follows.
``` shell
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0000.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0001.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0002.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0003.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0004.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0005.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0006.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0007.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0008.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0009.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0010.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0011.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0012.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0013.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0014.bin
scratch/rn/traces/HPCG/dumpi-2023.03.31.06.35.04-0015.bin
```

2. The AMR, IMB, LULESH, miniFE, Nekbone trace files are already in `scrach/rn/traces` as well.

3. If you want to replay DUMPI traces of other HPC applications, please follow the above file format.

### Compiling Simulator
1. The default initialization of the simulator is `scrach/rn/dragonfly-test.cpp`. You can make your own test of CoDES-based MPI.

2. Compile the simulator and then you will get the simultor.
``` shell
cd build
make simulator
```

### Running Simulator
1. Run the simulator with the HPCG application.
``` shell
cd build
./simulator ../scrach/rn/traces/HPCG
```
2. The provided test `scrach/rn/dragonfly-test.cpp` for CoDES-based MPI supports a range of typical and cutting-edge network functions. You can enable these network functions through different parameters.
Here's the parameters and descriptions:

| Parameter | Description |
|----------------------------|----------------------------------------------------------------------------------------------------------------|
| tracePath | Path containing binary dumpi traces |
| p | Number of servers per TOR switch |
| a | Number of TOR switches per group |
| h | Count of inter-group links per TOR switch |
| g | Number of groups; set to zero means using balanced dragonfly (g = a * h + 1) |
| bandwidth | Bandwidth of the links in the topology |
| delay | Delay of the links in the topology |
| ocs | Count of OCS used in the reconfigurable topology, each OCS is connected to a TOR in every group |
| ugal | Whether to use UGAL routing |
| flowRouting | Whether to use flow routing |
| congestionMonitorPeriod | Congestion monitor interval |
| enable_reconfiguration | Whether to enable reconfiguration |
| reconfiguration_timestep | Reconfiguration interval |
| stop_time | Time to stop generating flows, if synthetic traffic is enabled |
| is_adversial | Whether the traffic pattern is adversarial or neighbor, if synthetic traffic is enabled |
| ecmp | Whether to enable ECMP |
| app_bd | Bandwidth injected into the network by synthetic traffic, if synthetic traffic is enabled |
| bias | Bias for UGAL to determine the preference for adaptive routing or shortest path routing |
| reconfiguration_count | Maximum number of reconfigurations |
| only_reconfiguration | Whether the dragonfly has no background links |


## Brief Introduction
CoDES is mainly implemented in `src/coroutine` under the `master` branch. The following sections will introduce how CoDES implement coroutine support in DES, as well as how to enable specific suspendable network operations, through `CoroutineOperation` and `CoroutineSocket`.

### CoroutineOperation
In `src/coroutine/model/operation.h`, the struct `Promise` is responsible for preserving the local variables defined within the coroutine, the current execution position, the coroutine's call stack, and references to certain resources. The coroutine frame structure can be properly released by the design of acyclic resource reference graph.

In `src/coroutine/model/operation.h`, the `CoroutineOperation` can encapsulate any asynchronous network operation, allowing execution to pause and resume. The `CoroutineOperation` supports more user-friendly operations by the design of `OpearionCalculation` and `ChainOperation`. The `CoroutineOperation` consists of several components:
- The **coroutine frame structure** that defines how to manage the state, return values, and exception handling, as described in the previous section;
- The **coroutine handle** that allows the operation to be resumed or destroyed;
- The **execution logic** that encompasses the specific tasks required to complete the operation;
- The **suspension points** where developers can proactively define when the operation should suspend and resume, rather than being passively scheduled by the system;
- The **return values** and **exception handling**, where the return values are commonly provided at the end of the coroutine and passed back to the caller.

### CoroutineSocket
In `src/coroutine/model/socket`，the `CoroutineSocket` is seamlessly compatible with the original NS-3 socket and maintains four additional queues—`AcceptOperationQueue`, `ConnectOperationQueue`, `SendOperationQueue`, and `ReceiveOperationQueue`—for accepting incoming connections, handling outgoing connection requests, sending data, and receiving data respectively. 

## CoDES-based MPI
CoDES-based MPI is implemented in `src/mpi-application` under the `MPI` branch. 

- **mpi-application**: is seamlessly compatible with the original NS-3 application and can manage the creation, initialization, execution, blocking behavior, and destruction of the communicator.
- **mpi-communicator**: supports both point-to-point and collective communication operations like Ring AllReduce in MPI, and allows querying communicator parameters, such as the number of bytes transmitted or received and the size of the communication group.
- **mpi-functions**: translate DUMPI trace into corresponding MPI communication operations.
- **mpi-datatype**: supports different data type in MPI.
- **mpi-exception**: handles exceptions in MPI.

## CoDES-based HPCC
CoDES-based HPCC is implemented in `src/rdma` under the `RDMA` branch. 

- **rdma-device-driver**: handles PFC (Priority Flow Control) pause and resume operations.
- **rmda-node-device**: monitors and responds to PFC pause frames from network devices, and manages and updates pause timers for each interface.
- **infiniband-transport-header**: defines the packet headers used in Infiniband transport, supports building, modifying, and parsing Infiniband protocol packets.
- **pfc-header**: defines the data structure representing the PFC header and defines related operations.
- **pfc-queue-item**: encapsulates the packet, address, protocol, and PFC header, providing a dedicated item for queuing and facilitating PFC-aware traffic management. It also offers methods to add, access, mark and hash PFC headers and packets, all designed to streamline the simulation of PFC functionalities like header generation and packet handling during queuing.
- **pfc-tag**: enables the attachment of PFC-related metadata and provides the mechanisms for attaching, serializing, and accessing this information.


## CoDES-based RIP
CoDES-based RIP is implemented in `src/rip` under the `RIP` branch. 

- **rip-CoDES**: defines `RipRoutingTableEntry` for managing routing table entries, and `Rip`, which implements the RIP protocol, including route initialization, updates, reception, transmission, and selection. It incorporates methods for table management (addition, deletion, and lookup), supports both triggered and periodic updates, implements split horizon strategies to avoid routing loops, manages packet routing via `RouteOutput()` and `RouteInput()`, and provides interfaces for handling interface status and address changes.
- **rip-header-CoDES**: defines the data structures for the RIP protocol's header formats, encompassing both route table entries (RTEs) and the RIP header itself. `RipRte` represents a RIP route table entry, encapsulating information such as prefix, subnet mask, route tag, metric, and next hop. `RipHeader` represents the RIP protocol header, containing the command type (request or response) and a list of `RipRte` instances. 

## Tips
- **Recommended compiler**: GCC-13 and later versions. Previous versions may have some bugs that could cause errors.

## Credit
This repository contains code from the repository of NS-3:
* [NS-3](https://github.com/nsnam/ns-3-dev-git/tree/master)
