/*
	Processor Core Class
*/

#include "structs.hpp"
#include <stdint.h>

class core {
public:
	core();
	~core();

	void execute_instr(instr I);

private:
	uint8_t core_id;
	
};
