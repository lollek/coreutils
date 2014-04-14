#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>

#define BUFLEN 256

const char *progname = NULL;

int version(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout, "Lollek cat v0.1\n");
  return status;
}

int usage(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "Concatenate FILE(s), or standard input, to standard output.\n\n"
      "  -b, --number-nonblank     number nonempty output lines, overrides -n\n"
      "  -E, --show-ends           display $ at end of each line\n"
      "  -n, --number              number all output lines\n"
      "      --help                display this help and exit\n"
      "      --version             output version information and exit\n\n"
      "With no FILE, or when FILE is -, standard input is read\n"
      , progname);
  return version(status);
}

int print_file(const char *filename, int option_flags) {
  FILE *fd = NULL;
  if (!strcmp(filename, "-")) {
    fd = stdin;
  } else {
    if ((fd = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
      return 1;
    }
  }

  char buf[BUFLEN];
  unsigned line_counter = 0;

  bool number_lines = false;
  if (option_flags & 0x1) { number_lines = true; }
  bool number_nonblanks = false;
  if (option_flags & 0x4) { number_lines = false; number_nonblanks = true; }
  bool show_ends = false;
  if (option_flags & 0x2) { show_ends = true; }

  /* With flags */
  if (number_lines || number_nonblanks || show_ends) {
    while (fgets(buf, BUFLEN, fd) != NULL) {
      const char *tmpbuf = buf;
      for (char *c = buf; *c != '\0'; ++c) {
        if (*c == '\n') {
          *c++ = '\0';
          if (number_lines || (number_nonblanks && *tmpbuf != '\0')) {
            printf("%6d  ", ++line_counter);
          }
          printf("%s", tmpbuf);
          if (show_ends) {
            putchar('$');
          }
          putchar('\n');
          tmpbuf = c;
        }
      }
      if (*tmpbuf != '\0') {
        if (number_lines || number_nonblanks) {
          printf("%6d  ", ++line_counter);
        }
        printf("%s", tmpbuf);
      }
    }

  /* Without any flags */
  } else {
    while (fgets(buf, BUFLEN, fd) != NULL) {
      printf("%s", buf);
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  progname = argv[0];
  int option_flags = 0;
  int option_index = 0;
  static struct option long_options[] = {
    {"number-nonblank", no_argument, 0, 'b'},
    {"show-ends",       no_argument, 0, 'E'},
    {"number",          no_argument, 0, 'n'},
    {"help",            no_argument, 0,  0},
    {"version",         no_argument, 0,  1}
  };

  while (1) {
    int c = getopt_long(argc, argv, "bEn", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'b': option_flags |= 0x4; break;
      case 'E': option_flags |= 0x2; break;
      case 'n': option_flags |= 0x1; break;
      case  0: return usage(0);
      case  1: return version(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n", 
                progname);
        return 1;
    }
  }

  if (optind == argc) {
    print_file("-", option_flags);
  } else {
    for (int i = optind; i < argc; ++i) {
      print_file(argv[i], option_flags);
    }
  }
  return 0;
}
