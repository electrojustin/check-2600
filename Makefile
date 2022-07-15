CC=g++ -g -lstdc++ -I/usr/include/qt -I/usr/include/qt/QtGui -I/usr/include/qt/QtCore -I/usr/include/qt/QtWidgets -lQt5Core -lQt5Gui -lQt5Widgets
atari2600: main.o registers.o memory.o operand.o instructions.o cpu.o qt_display.o display.o ntsc.o tia.o atari.o
	${CC} main.o registers.o memory.o operand.o instructions.o cpu.o qt_display.o display.o ntsc.o tia.o atari.o -o atari2600
main.o: main.cc atari.h
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
display.o: display.cc display.h qt_display.h
	${CC} -c display.cc
qt_display.o: display.h qt_display.cc qt_display.h
	${CC} -c qt_display.cc
ntsc.o: ntsc.cc ntsc.h display.h
	${CC} -c ntsc.cc
tia.o: tia.cc tia.h ntsc.h registers.h memory.h
	${CC} -c tia.cc
atari.o: atari.cc atari.h tia.h memory.h registers.h cpu.h
	${CC} -c atari.cc
clean:
	rm *.o
