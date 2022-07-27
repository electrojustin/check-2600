#include <QApplication>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "atari.h"

void print_usage_and_exit() {
  printf("Usage: atari2600 [--debug] <program_file>\n");
  exit(-1);
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  char *filename;
  bool debug = false;

  if (argc < 2 || argc > 3) {
    printf("Wrong number of arguments!\n");
    print_usage_and_exit();
  }

  if (argc == 3) {
    if (!strncmp(argv[1], "--debug", strlen("--debug"))) {
      debug = true;
      filename = argv[2];
    } else if (!strncmp(argv[2], "--debug", strlen("--debug"))) {
      debug = true;
      filename = argv[1];
    } else {
      printf("Error! Unrecognized argument!\n");
      print_usage_and_exit();
    }
  } else {
    filename = argv[1];
  }

  load_program_file(filename);

  start_emulation_thread(debug);

  return app.exec();
}
