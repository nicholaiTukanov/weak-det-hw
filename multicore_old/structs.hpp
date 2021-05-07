/*
    This file will hold all the structs/types that will be used 
    in our custom simulator.
*/

#include <stdlib.h>
#include <stdint.h>
#include <unordered_map>
using namespace std;

/*
    we use a global unordered hash map to store each lock entry
    this provides us with O(1) store/access

    the map will have the following layout:

    ptr_to_resource = lock_entry

 */
std::unordered_map<void* , struct lock_entry> lock_map;

// code that will populate the hash map for the locks'
// lock_map[ptr] = lock_entry

typedef uint32_t thread_t;

// a struct for the metadata of an instruction
typedef struct {
    int instr_id; 
    int thread_id; 
    int timestamp; 
    void *program_counter;
    void *pointer_to_resource; 
} instr;

struct lock_entry {
    //void *resource;
    void *pointer_to_resource;
    size_t bytes;
    thread_t thread_id;
};

// lock table contains only active locks (dynamic) (linked list) (map library)
// lock table entry. max locks >= number of critical sections
// overlapping locks
// lock entry: <pointer to resource that is locked> <size of resource in bytes> <thread_Id>
// <pointer to resource> is the key

// instr.pointer_to_resource lies in (pointer_resource_in lock_entry + bytes) 

/*

General Idea:

LOCK ACQ <- // get a timestamp, get ptr of acq
.
.
.
LOCK RELEASE <- // ptr + bytes

*/