#include "control.hpp"
#include "structs.hpp"

// initalization funciton
control::control(core array_cores) {
    

}

// garabage collection for control objects
control::~control() {
    
    
}

// this function will search our lock map using the ptr to the resource that the instruction I is targeting.
// if there exists an entry in our map that has the ptr as a key, 
// then we return false since the instruction is trying to access a resource that is being held by another thread
// otherwise, we return true since the instruction can execute without worry
bool control::legal_instruction(instr I) {

    if ( lock_map.find(I.pointer_to_resource) == lock_map.end()) {
        return true;
    }
    else {
        return false;
    }
}

// this module will decide whether it is LEGAL to pass an instruction to a given core
// in the case that an instruction can not be passed (i.e. this instruction is attempting to compute on some resource that is guarded by a lock)
// then the control module will send NOPS to the core until the instruction is allowed to be processed
void control::control_module(instr I) {
    
    if ( control::legal_instruction (I) ) {
        // pass the instruction to a given core passed on the metadata of the instruction
        // pass the instruction to the ith core from array_cores. (execute_instructin<instruction_struct>)
        // pass to the core only if it is free
    }
    else {
        // pass nops to the core that is stated by the metadata of the instruction
    }

}

