#include "control.hpp"

// initalization funciton
control::control(core array_cores) {
    

}

// garabage collection for control objects
control::~control() {
    
    
}

/*
struct instruction {
    //instruction details -operands, function, latency 
    timestamp <- done by the scheduler
    core assigned to 
}
*/
// this module will decide whether it is LEGAL to pass an instruction to a given core
// in the case that an instruction can not be passed (i.e. this instruction is attempting to compute on some resource that is guarded by a lock)
// then the control module will send NOPS to the core until the instruction is allowed to be processed
void control::control_module() {
    
    if ( LEGAL_INSTRUCTION (INSTRUCTION) ) {
        // pass the instruction to a given core passed on the metadata of the instruction
        // pass the instruction to the ith core from array_cores. (execute_instructin<instruction_struct>)
    }
    else {
        // pass nops to the core that is stated by the metadata of the instruction
    }

}

