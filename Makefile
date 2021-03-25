general_compile_options = -Werror -Wextra -Wall 
style_compile_options = -Wparentheses -pedantic -Wunused-variable
variable_compile_options = -Wfloat-equal -Wconversion -Wunused-parameter -Wredundant-decls -Wunused-value -Wuninitialized  
func_compile_options = -Wreturn-type -Wunused-function -Wswitch-default  -Winit-self


INCLUDE := ./include


CC 		:= g++
CFLAGS 	:= -O3 -g $(INCLUDE) $(general_compile_options) #$(style_compile_options) $(variable_compile_options) $(func_compile_options)

DEPS = multicore/core.hpp multicore/control.hpp
OBJ = multicore/core.o multicore/control.o

all: main.o $(OBJ)
	$(CC) main.o $(OBJ) -o main

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: main.o $(OBJ)
	rm -rf *o
	rm -rf multicore/*o

	chmod a+x main
	./main

clean:
	rm -rf *o main
	rm -rf multicore/*o