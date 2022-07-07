CC=g++ -g -std=gnu++11
atari2600: main.o registers.o memory.o operand.o instructions.o cpu.o
	${CC} main.o registers.o memory.o operand.o instructions.o cpu.o -o atari2600
main.o: main.cc cpu.h registers.h memory.h atari.h
	${CC} -c main.cc
registers.o: registers.h registers.cc
	${CC} -c registers.cc
memory.o: memory.h memory.cc
	${CC} -c memory.cc
operand.o: operand.h operand.cc registers.h memory.h
	${CC} -c operand.cc
instructions.o: instructions.h instructions.cc registers.h memory.h cpu.h
	${CC} -c instructions.cc
cpu.o: cpu.h cpu.cc operand.h instructions.h registers.h memory.h
	${CC} -c cpu.cc
clean:
	rm *.o
