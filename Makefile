CC=g++ -g -lstdc++ -I/usr/include/qt -I/usr/include/qt/QtGui -I/usr/include/qt/QtCore -I/usr/include/qt/QtWidgets -lQt5Core -lQt5Gui -lQt5Widgets
atari2600: main.o registers.o memory.o operand.o instructions.o cpu.o qt_display.o
	${CC} main.o registers.o memory.o operand.o instructions.o cpu.o qt_display.o -o atari2600
main.o: main.cc cpu.h registers.h memory.h atari.h qt_display.h
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
qt_display.o: display.h qt_display.cc qt_display.h
	${CC} -c qt_display.cc
clean:
	rm *.o
