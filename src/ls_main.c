#include <stdio.h>
#include <getopt.h>        /* getopt_long */

#include "ls.h"

const char *progname = NULL;

int main(int argc, char **argv) {
  int option_index = 0;
  struct option long_options[] = {
    {"help",    no_argument, 0, 0},
    {"version", no_argument, 0, 1},
    {0,         0,           0, 0}
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "0", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0: return usage(0);
      case 1: return version(0);
      default: fprintf(stderr, "Try '%s --help' for more information\n", 
                       progname);
               return 1;
    }
  }

  if (optind == argc) {
    return print_path(".");
  } else if (argc - optind == 1) {
    return print_path(argv[1]);
  } else {
    return ls(argc - optind, (const char **) &argv[optind]);
  }
}

