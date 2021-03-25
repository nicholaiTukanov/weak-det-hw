#include "core.hpp"

core::core() {

}

core::~core() {


}

// this method will execute the instruction I
void core::execute_instr(instr I) { 
    // set is_free to false
    // check if core id is matching
    // Retrive clock cycles from instruction id
    // wait for appropriate clock cycles associated with the instruction
    // before returning, release the lock (i.e. update lock table)
    // set is_free to true
}
