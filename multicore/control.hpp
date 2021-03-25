/*
	Control Module Class
*/


class control {
public:
	control();
	~control();

	void control_module(instr);
	bool legal_instruction(instr);

	bool is_free = true;

private:
	
};