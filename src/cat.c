#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#define BUFLEN 256

const char *progname = NULL;

int usage(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout,
      "Usage: %s\n", progname);
  return status;
}

int print_file(const char *filename) {
  FILE *fd = fopen(filename, "r");
  if (fd == NULL) {
    fprintf(stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
    return 1;
  }

  char buf[BUFLEN];
  while (fgets(buf, BUFLEN, fd) != NULL) {
    printf("%s", buf);
  }
  return 0;
}

int main(int argc, char **argv) {
  progname = argv[0];

  /* If no arguments - just print the stdin */
  if (argc == 1) {
    char buf[BUFLEN];
    while (fgets(buf, BUFLEN, stdin) != NULL) {
      printf("%s", buf);
    }
    return 0;
  }

  int c;
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"number", no_argument, 0, 'n'},
      {"help",   no_argument, 0,  0}
    };

    c = getopt_long(argc, argv, "n", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0: return usage(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n", 
                progname);
        return 1;
    }
  }
  /* Concat all files in argc */
  for (int i = 1; i < argc; ++i) {
    print_file(argv[i]);
  }
  return 0;
}
