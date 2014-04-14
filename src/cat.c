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
  fprintf(status ? stderr : stdout, "Lollek cat v1.0\n");
  return status;
}

int usage(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "Concatenate FILE(s), or standard input, to standard output.\n\n", 
      progname);
  fprintf(status ? stderr : stdout,
      "  -b, --number-nonblank    number nonempty output lines, overrides -n\n"
      "  -E, --show-ends          display $ at end of each line\n"
      "  -n, --number             number all output lines\n"
      "  -s, --squeeze-blank      suppress repeated empty output lines\n"
      "  -T, --show-tabs          display TAB characters as ^I\n"
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n"
      "With no FILE, or when FILE is -, standard input is read\n");
  return version(status);
}

int print_file(const char *filename, unsigned option_flags) {
  static unsigned line_counter = 0;
  static bool last_line_was_blank = false;
  static bool prevent_enumeration = false;
  FILE *fd = NULL;
  char buf[BUFLEN];

  if (!strcmp(filename, "-")) {
    fd = stdin;
  } else {
    if ((fd = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
      return 1;
    }
  }

  /* With flags */
  if (option_flags & 0xFFFFFFFF) {
    while (fgets(buf, BUFLEN, fd) != NULL) {
      char *c = buf;
      const char *bufptr = buf;

      /* Squeeze blanks */
      if (option_flags & 0x8) {
        if (buf[0] == '\n' && buf[1] == '\0') {
          if (last_line_was_blank) {
            continue;
          }
          last_line_was_blank = true;
        } else {
          last_line_was_blank = false;
        }
      }

      /* Enumerate lines */
      if (!prevent_enumeration && buf[0] != '\0' &&
          ((option_flags & 0x1 && ~option_flags & 0x4) ||
           (option_flags & 0x4 && buf[0] != '\n'))) {
        printf("%6d\t", ++line_counter);
      }

      /* Show tabs */
      if (option_flags & 0x10) {
        while (*c != '\0' && *c != '\n') {
          if (*c == '\t') {
            *c = '\0';
            printf("%s^I", bufptr);
            bufptr = ++c;
          } else {
            ++c;
          }
        }
      } else {
        while (*c != '\0' && *c != '\n')
          ++c;
      }

      if (*c == '\n') {
        *c = '\0';
        /* Show ends */
        if (option_flags & 2) {
          printf("%s$\n", bufptr);
        } else {
          printf("%s\n", bufptr);
        }
        prevent_enumeration = false;
      } else {
        printf("%s", bufptr);
        prevent_enumeration = true;
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
  unsigned option_flags = 0;
  int option_index = 0;
  struct option long_options[] = {
    {"number-nonblank", no_argument, 0, 'b'},
    {"show-ends",       no_argument, 0, 'E'},
    {"number",          no_argument, 0, 'n'},
    {"squeeze-blank",   no_argument, 0, 's'},
    {"show-tabs",       no_argument, 0, 'T'},
    {"help",            no_argument, 0,  0},
    {"version",         no_argument, 0,  1}
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "bEnsT", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'b': option_flags |= 0x4; break;  /* Number nonblank */
      case 'E': option_flags |= 0x2; break;  /* Show ends */
      case 'n': option_flags |= 0x1; break;  /* Number lines */
      case 's': option_flags |= 0x8; break;  /* Squeeze blanks */
      case 'T': option_flags |= 0x10; break; /* Show tabs */
      case  0: return usage(0);              /* Help */
      case  1: return version(0);            /* Version */
      default:
        fprintf(stderr, "Try '%s --help' for more information\n", 
                progname);
        return 1;
    }
  }

  if (optind == argc) {
    print_file("-", option_flags);
  } else {
    int i;
    for (i = optind; i < argc; ++i) {
      print_file(argv[i], option_flags);
    }
  }
  return 0;
}
