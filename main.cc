#include <QApplication>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "atari.h"
#include "bank_switchers.h"

void print_usage_and_exit() {
  printf("Usage: atari2600 [-d] [-s scale]  -f <program_file>\n");
  printf("-d: Enter debug mode.\n");
  printf("-s: Set UI scale. Default is 4.\n");
  printf("-h: Show this help menu and exit.\n");
  exit(0);
}

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  char *filename = nullptr;
  bool debug = false;
  int scale = 4;
  BankSwitcherType bank_switcher_type = BankSwitcherType::none;

  int c;
  while ((c = getopt(argc, argv, "hds:f:b:")) != -1) {
    switch (c) {
    case 'h':
      print_usage_and_exit();
      break;
    case 'd':
      debug = true;
      break;
    case 's':
      scale = atoi(optarg);
      if (scale <= 0) {
        printf("Error! Invalid scale %d\n", scale);
        exit(-1);
      }
      break;
    case 'f':
      filename = (char *)malloc(strlen(optarg) + 1);
      strcpy(filename, optarg);
      break;
    case 'b':
      if (!strncmp(optarg, "atari8k", strlen("atari8k"))) {
        bank_switcher_type = BankSwitcherType::atari8k;
      } else if (!strncmp(optarg, "atari16k", strlen("atari16k"))) {
        bank_switcher_type = BankSwitcherType::atari16k;
      } else if (!strncmp(optarg, "atari32k", strlen("atari32k"))) {
        bank_switcher_type = BankSwitcherType::atari32k;
      } else {
        printf("Error! Invalid bankswitch type %s\n", optarg);
        exit(-1);
      }
      break;
    default:
      printf("Invalid argument %c\n", c);
      print_usage_and_exit();
    }
  }

  if (!filename)
    print_usage_and_exit();

  load_program_file(filename, scale, bank_switcher_type);

  start_emulation_thread(debug);

  free(filename);

  return app.exec();
}
