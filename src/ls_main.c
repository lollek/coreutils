#include <stdio.h>
#include <string.h>       /* strerror */
#include <getopt.h>       /* getopt_long */
#include <errno.h>        /* errno */
#include <sys/stat.h>     /* stat */

#include "ls.h"

const char *progname = NULL;
ls_sort_t sorting = COLL;
list_which_files_t list_which_files = NOT_HIDDEN;

int version(int status) {
  fprintf(status ? stderr : stdout, "lollek-coreutils/ls v0.2b\n");
  return status;
}

int usage(int status) {
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "List information about FILE (the current directory by default)\n"
      "The entires are sorted alphabetically by default\n\n",
      progname);
  fprintf(status ? stderr : stdout,
      "  -a, --all                do not ignore entries starting with .\n"
      "  -A, --almost-all         do not list implied . and ..\n"
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n");
  return version(status);
}

int main(int argc, char **argv) {
  int option_index = 0;
  struct option long_options[] = {
    {"all",            no_argument, 0, 'a'},
    {"almost-all",     no_argument, 0, 'A'},
    {"help",           no_argument, 0,  0},
    {"version",        no_argument, 0,  1},
    {0,                0,           0,  0}
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "aA", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'a': list_which_files = ALL; break;
      case 'A': list_which_files = ALMOST_ALL; break;
      case  0: return usage(0);
      case  1: return version(0);
      default: fprintf(stderr, "Try '%s --help' for more information\n", 
                       progname);
               return 1;
    }
  }

  if (optind == argc) {
    return print_path(".", NULL);
  } else if (argc - optind == 1) {
    int status;
    struct stat pathstat;
    if (stat(argv[1], &pathstat) == -1) {
      fprintf(stderr, "%s: cannot access %s: %s\n",
              progname, ".", strerror(errno));
      return 1;
    }
    status = print_path(argv[1], &pathstat);
    if (status == 0 && !S_ISDIR(pathstat.st_mode)) {
      putchar('\n');
    }
    return status;
  } else {
    return ls(argc - optind, (const char **) &argv[optind]);
  }
}

