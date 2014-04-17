#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>

#define BUFLEN 256

#include "cat_strlst.h"

static const char *progname = NULL;

/* If output_type is HEAD, we will only print num_lines many lines, then quit
 * If output_type is TAIL, we will only print the last num_lines lines
 * If output_type is NONE, we will print to stdout */
static int num_lines = -1;
static enum output_type_t {
  NONE, HEAD, TAIL
} output_type = NONE;

/* Command line argumnets */
static bool number_lines = false;
static bool number_nonblanks = false;
static bool show_ends = false;
static bool show_tabs = false;
static bool show_nonprinting = false;
static bool squeeze_blanks = false;

int version(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout, "lollek-coreutils/cat v1.2e\n");
  return status;
}

int usage(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "Concatenate FILE(s), or standard input, to standard output\n\n", 
      progname);
  fprintf(status ? stderr : stdout,
      "  -A, --show-all           equivalent to -vET\n"
      "  -b, --number-nonblank    number nonempty output lines, overrides -n\n"
      "  -e                       equivalent to -vE\n"
      "  -E, --show-ends          display $ at end of each line\n"
      "  -l, --lines=[-]N         print only the first/last N lines (default 10)\n");
  fprintf(status ? stderr : stdout,
      "  -n, --number             number all output lines\n"
      "  -p, --page=N             start paging after N lines (default 40)\n"
      "                             this option overrides -l\n"
      "  -s, --squeeze-blank      suppress repeated empty output lines\n"
      "  -t                       equivalent to -vT\n"
      "  -T, --show-tabs          display TAB characters as ^I\n"
      "  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n"
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n"
      "With no FILE, or when FILE is -, standard input is read\n");
  return version(status);
}

void lprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (output_type == TAIL) {
    if (strlst_num_nodes() == num_lines) {
      strlst_shift();
    }
    char buf[BUFLEN];
    vsnprintf(buf, BUFLEN, format, args);
    buf[BUFLEN -1] = '\0';
    strlst_push(buf);
  } else {
    vprintf(format, args);
  }
  va_end(args);
}

int print_file(const char *filename) {
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
  if (number_lines || number_nonblanks || show_ends || show_tabs ||
      show_nonprinting || squeeze_blanks || output_type != NONE) {
    while (fgets(buf, BUFLEN, fd) != NULL) {
      char *c = buf;
      const char *bufptr = buf;

      if (squeeze_blanks) {
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
          ((number_lines && !number_nonblanks) ||
           (number_nonblanks && buf[0] != '\n'))) {
        lprintf("%6d\t", ++line_counter);
      }

      if (show_tabs || show_nonprinting) {
        while (*c != '\0' && *c != '\n') {
          if (*c == '\t' && show_tabs) {
            *c = '\0';
            lprintf("%s^I", bufptr);
            bufptr = ++c;
          } else if (show_nonprinting && *c != '\t') {
            char char_to_print[5] = {0, 0, 0, 0, 0};
            if (*c < 32) { 
              char_to_print[0] = '^';
              char_to_print[1] = 64 + *c;
            } else if (*c < 127) {
              ++c;
              continue;
            } else if (*c == 127) {
              char_to_print[0] = '^';
              char_to_print[1] = '?';
            } else if (*c > 127) {
              char_to_print[0] = 'M';
              char_to_print[1] = '-';
              if (*c < 160) {
                char_to_print[2] = '^';
                char_to_print[3] = *c - 96;
              } else if (*c < 255) {
                char_to_print[2] = *c - 128;
              } else {
                char_to_print[2] = '^';
                char_to_print[3] = '?';
              }
            }
            *c = '\0';
            lprintf("%s%s", bufptr, char_to_print);
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
        lprintf(show_ends? "%s$\n" : "%s\n", bufptr);

        /* If we only print N lines - decrease the lines left */
        if (output_type == HEAD && --num_lines <= 0) {
          break;
        }
        prevent_enumeration = false;
      } else {
        lprintf("%s", bufptr);
        prevent_enumeration = true;
      }
    }

  /* Without any flags */
  } else {
    while (fgets(buf, BUFLEN, fd) != NULL) {
      printf("%s", buf);
    }
  }
  fclose(fd);
  return 0;
}

int main(int argc, char **argv) {
  int option_index = 0;
  struct option long_options[] = {
    {"show-all",         no_argument,       0, 'A'},
    {"number-nonblank",  no_argument,       0, 'b'},
    {"show-ends",        no_argument,       0, 'E'},
    {"lines",            optional_argument, 0, 'l'},
    {"number",           no_argument,       0, 'n'},
    {"page",             optional_argument, 0, 'p'},
    {"squeeze-blank",    no_argument,       0, 's'},
    {"show-tabs",        no_argument,       0, 'T'},
    {"show-nonprinting", no_argument,       0, 'v'},
    {"help",             no_argument,       0,  0 },
    {"version",          no_argument,       0,  1 },
    {0,                  0,                 0,  0 }
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "AebEl::np::stTv",
                        long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'A': show_ends = true; show_tabs = true; 
                show_nonprinting = true; break;
      case 'b': number_nonblanks = true;  break;
      case 'e': show_ends = true; show_nonprinting = true; break;
      case 'E': show_ends = true; break;
      case 'l': if (optarg == NULL) {
                  output_type = HEAD;
                  num_lines = 10;
                } else if (optarg[0] == '-' && optarg[1] == '\0') {
                  output_type = TAIL;
                  num_lines = 10;
                } else {
                  num_lines = atoi(optarg);
                  output_type = num_lines >= 0 ? HEAD : TAIL;
                } break;
      case 'n': number_lines = true; break;
      case 's': squeeze_blanks = true; break;
      case 't': show_tabs = true; show_nonprinting = true; break;
      case 'T': show_tabs = true; break;
      case 'v': show_nonprinting = true; break;
      case  0: return usage(0);
      case  1: return version(0);
      default:
        fprintf(stderr, "Try '%s --help' for more information\n", 
                progname);
        return 1;
    }
  }

  if (optind == argc) {
    print_file("-");
  } else {
    int i;
    /* NB: strlst_max_lines is a global which in some cases is decreased in
     * print_file (it acts as "lines left" when the -l flag is set) */
    for (i = optind; i < argc && num_lines != 0; ++i) {
      print_file(argv[i]);
    }
  }
  if (output_type == TAIL) {
    strlst_flush();
  }
  return 0;
}
