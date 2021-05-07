/*
	Control Module Class
*/
#inclide "core.hpp"

class control {
public:
	control(core *all_cores);
	~control();

	void control_module(instr);
	bool legal_instruction(instr);

	bool is_free = true;

private:
	
};