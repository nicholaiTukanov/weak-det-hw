# An Exploration of Weak Determinism in Hardware
An exploration of hardware level weak determinism.

### Introduction
-------------------------------------------------
We present a custom x86 simulator that is meant to implement weak determinism in hardware. We plan to use this simulator in conjuction with Pin, Intel's binary instrumentation tool.

### Simulator Details
-------------------------------------------------
Our simulator emulates a multicore system where each core will process some instruction stream that is determined by a scheduling algorithm. Each core will only run an instruction that has been approved by a control module.

### Hardware Weak Determinism
-------------------------------------------------
To ensure that our simulator has weak determinism, we must deterministically order the accesses to the critical sections of a given program.

Our idea is to have a scheduling algorithm that will pack metadata into each instruction. This metadata will be stored into a typedefined instruction struct. Once the scheduling algorithm has created the metadata for the instruction, it will pass the instruction and metadata to the control module. The control module will then ensure that we have a deterministic order on the accesses to some critical section.

The control module deterministically orders the accesses to a critical section by checking the metadata of the instruction. Once the control module reads the metadata of the instruction, it will use the information provided to check a global lock table that is implemented in hardware.

Here the control module checks the global lock table to ensure that the instruction is not accessing a critical section. If the instruction is not accessing a critical section, then the instruction is allowed to be executed by some core that is determined by the scheduling algorithm. If the instruction is accessing a critical section of a program, then the control module will check to see if another thread is currently in the critical section. This checking happens by searching the global lock table to see if there exists an entry for that given critical section. The search of the global lock table is implemented by checking if the pointer of the instruction is exclusively in the range of a critical section (i.e. the pointer of the instruction is not in the range of the pointer to the start of a critical section plus the size of the critical section). If the instruction is in the range of a critical section that is NOT in the global lock table, then an entry is added to the lock table and the control module allows the executation of the instruction. If the instruction is in the range of a critical section that is in the global lock table, then we pass NOPs to the cores until we see that thread in the critical section has released the lock. 




<!-- After the scheduling algorithm has packed each instruction with the respective metadata, the instruction stream is passed to a control module. The control module is the authority that will check whether or not the instruction is allowed to be executed on a core that is selected by the scheduling algorithm. The control module determines if an instruction can legally execute by ensuring that the instruction is not a part of some critical section that is being executed by some thread.  -->




### Enviornment Info
-------------------------------------------------
* GCC/G++ version 9.3.0
* C++ version 201402L
* WSL with Ubuntu 20.04
* Architecture: Zen2 (AMD Ryzen 5 3600x)
