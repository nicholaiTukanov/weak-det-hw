# An Exploration of Weak Determinism in Hardware
A project on the exploration of hardware level weak determinism for Professor Brandon Lucia's 18-742 course from Carnegie Mellon University.

**Note: This simulator is a work in progress, so the documentation for this project has not been fully completed.**

### Introduction
-------------------------------------------------
The majority of the DMT systems (i.e. Kendo, CoreDet, and LazyDet) are fully software-based solutions. Since they are software-based, these DMT solutions provide programmers with flexible frameworks that can be easily accessed and understood. However, by implementing the solution fully in software, the trade off is that the performance of the system degrades. For this project, we plan on exploring the implications of implementing a hardware-based DMT system that uses weak determinism. Our end goal is to describe the implementation details for a weak hardware-based DMT system, and to show whether or not a weak hardware-based DMT system will provide a performance improvement over state of the art software DMT systems such as LazyDet.

We attempt our hardware weak DMT system by implementing a custom x86 simulator written mostly in C++. For the rest of this document, we provide high-level details on our simulator, our methodology for ensuring weak determinism, and the implemention details on our methodology.

### Simulator Details
-------------------------------------------------
Our simulator emulates a multicore x86 system where each core will process some instruction stream that has been provided by a scheduling algorithm. Before a core executes an instruction, it must be approved by a control module. This control module is where we deterministically order accesses to critical sections of a program. 

### Hardware Weak Determinism
-------------------------------------------------
To ensure that our simulator has weak determinism, we must deterministically order the accesses to the critical sections of a given program. Since our goal is to implement it in hardware, we attempt to achieve this by creating a global lock table in hardware that stores lock entries each time some instruction of some thread accesses a critical section of a program. We discuss the implementation details of our procedure for determining the order of accesses in the next section.


### Implementation Details
-------------------------------------------------

Our idea is to have a scheduling algorithm that will pack metadata into each instruction. This metadata will be stored into a typedefined instruction struct. 

```
typedef struct {
    int instr_id; 
    int thread_id; 
    int timestamp; 
    void *program_counter;
    void *pointer_to_resource; 
} instr;
```

Once the scheduling algorithm has created the metadata for the instruction, it will pass the instruction and metadata to the control module. The control module will then ensure that we have a deterministic order on the accesses to some critical section.

The control module deterministically orders the accesses to a critical section by checking the metadata of the instruction. Specifically, we examine the `timestamp` and `pointer_to_resource` that is stored in the `instr struct`. Once the control module reads the metadata of the instruction, it will use the information provided to check a global lock table that is implemented in hardware.

Here the control module checks the global lock table to ensure that the instruction is not accessing a critical section. If the instruction is NOT accessing a critical section, then the instruction is allowed to be executed by some core that is determined by the scheduling algorithm. If the instruction is accessing a critical section of a program, then the control module will check to see if another thread is currently in the critical section. This checking happens by searching the hardware global lock table to see if there exists an entry for that given critical section. The search of the global lock table is implemented by checking if the `pointer_to_resource` of the instruction is exclusively in the range of a critical section (i.e. the `pointer_to_resource` of the instruction is in the range of the pointer to the start of a critical section plus the size of the critical section). If the instruction is in the range of a critical section that is NOT in the global lock table, then an lock entry is added to the lock table and the control module allows the execution of the instruction. 

```
struct lock_entry {
    void *pointer_to_resource;
    size_t bytes;
    thread_t thread_id;
};
```

If the instruction is in the range of a critical section that is in the global lock table, then we pass NOPs to the cores until we see that thread in the critical section has released the lock. We provide a flowchart to showcase the logic that the control module follows:

<img title="Flow Chart for the logic of the Control Module" src="/images/weak_det_flowchart.png">


### Enviornment Info
-------------------------------------------------
Here we provide the toolchain/language versions/system data that we used for running our custom simulator.

* GCC/G++ version 9.3.0
* C++ version 201402L
* WSL with Ubuntu 20.04
* Architecture: Zen2 (AMD Ryzen 5 3600x)


### Authors
-------------------------------------------------
* Nicholai Tukanov (nicholaiTukanov) 
* Swarali Patil (Swarali20)
* Atharv Sathe (atharvsathe)

With assistance from 
* Professor Brandon Lucia
* Professor Joseph Devietti
* Kiwan Maeng

