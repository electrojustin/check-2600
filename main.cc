#include <QApplication>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory>

#include "atari.h"

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	if (argc != 2) {
		printf("invalid arguments!\n");
		printf("usage: atari2600 <program_file>\n");
		exit(-1);
	}

	load_program_file(argv[1]);

	start_emulation_thread();

	return app.exec();
}
