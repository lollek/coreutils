#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <getopt.h>

#define BUFLEN 256

/* max_num_lines is set in main() and decreased in print_file */
int max_num_lines = 10;

/** Linked list of chars and its functions
 * There are used to 'tail' a file | output since I've decided to include
 * that in my cat
 */
struct strlst {
  int id;
  bool has_newline;
  char data[BUFLEN];
  struct strlst *next;
} *strlst_start = NULL, *strlst_end = NULL;

/* Push a new line to list 
 * string cannot be larger than BUFLEN in size (including '\0') */
void strlst_push(const char *string) {
  int id = strlst_end == NULL ? 0 : strlst_end->id;
  bool has_newline = false;
  const char *strptr = string;

  /* Check if the string contains newlines */
  for (; *strptr != '\0'; ++strptr) {
    if (*strptr == '\n') {
      has_newline = true;
      if (id != max_num_lines) {
        ++id;
      }
      break;
    }
  }
  strptr = string;

  /* If neither it nor the previous string does, we can try to concat them.
   * If it works, we can return */
  if (!has_newline && strlst_end != NULL && !strlst_end->has_newline &&
      strlen(string) + strlen(strlst_end->data) < BUFLEN) {
    strcat(strlst_end->data, string);
    return;
  }

  /* Check if we've reached the max amount of lines to save, 
   *   and in that case we'll remove first line.
   * Sometimes with binary data, this will free the whole list, so watch out! */
  if (strlst_end != NULL && strlst_end->id == max_num_lines) {
    struct strlst *delptr = strlst_start;
    while (delptr != NULL) {
      bool break_next = delptr->has_newline;
      strlst_start = strlst_start->next;
      free(delptr);
      delptr = strlst_start;
      if (break_next) {
        break;
      }
    }
    if (strlst_start == NULL) {
      strlst_end = NULL;
    }
  }

  /* Add string */
  if (strlst_end == NULL) {
    strlst_start = (struct strlst *)malloc(sizeof(struct strlst));
    strlst_end = strlst_start;
  } else {
    strlst_end->next = (struct strlst *)malloc(sizeof(struct strlst));
    strlst_end = strlst_end->next;
  }
  if (strlst_end == NULL) {
    perror("malloc");
    exit(1);
  }

  strlst_end->id = id;
  strlst_end->has_newline = has_newline;
  strncpy(strlst_end->data, strptr, BUFLEN);
  strlst_end->next = NULL;
}

void strlst_print_all() {
  struct strlst *node = NULL;
  while (strlst_start != NULL) {
    node = strlst_start;
    strlst_start = strlst_start->next;
    printf("%s", node->data);
    free(node);
  }
  strlst_end = NULL;
}

/* My print function to make the code cleaner
 * When to_strlst is true, it appends the string to strlst
 * When to_strlst is false, it works like printf */
void lprintf(bool to_strlst, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (to_strlst) {
    char buf[BUFLEN];
    vsnprintf(buf, BUFLEN, format, args);
    buf[BUFLEN -1] = '\0';
    strlst_push(buf);
  } else {
    vprintf(format, args);
  }
  va_end(args);
}


/** End of linked list & friends **/


const char *progname = NULL;

int version(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout, "lollek-coreutils/cat v1.2c\n");
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
      "  -s, --squeeze-blank      suppress repeated empty output lines\n"
      "  -t                       equivalent to -vT\n"
      "  -T, --show-tabs          display TAB characters as ^I\n"
      "  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n"
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
        lprintf(option_flags & 0x20 && max_num_lines < 0,
                "%6d\t", ++line_counter);
      }

      /* Show tabs // nonprinting */
      if (option_flags & (0x10|0x40)) {
        while (*c != '\0' && *c != '\n') {
          if (*c == '\t' && option_flags &0x10) {
            *c = '\0';
            lprintf(option_flags & 0x20 && max_num_lines < 0,
                    "%s^I", bufptr);
            bufptr = ++c;
          } else if (option_flags &0x40 && *c != '\t') {
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
            lprintf(option_flags & 0x20 && max_num_lines < 0,
                    "%s%s", bufptr, char_to_print);
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
        lprintf(option_flags & 0x20 && max_num_lines < 0,
                option_flags & 2 ? "%s$\n" : "%s\n", bufptr);

        /* If we only print N lines - decrease the lines left */
        if (option_flags & 0x20) {
          if (max_num_lines > 0 && --max_num_lines == 0) {
            break;
          }
        }
        prevent_enumeration = false;
      } else {
        lprintf(option_flags & 0x20 && max_num_lines < 0,
                "%s", bufptr);
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
  unsigned option_flags = 0;
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
      case 'A': option_flags |= 0x2 | 0x10 | 0x40; break; /* Same as -vET */
      case 'b': option_flags |= 0x4;  break;  /* Number nonblank */
      case 'e': option_flags |= 0x2 | 0x40; break;  /* Same as -vE */
      case 'E': option_flags |= 0x2;  break;  /* Show ends */
      case 'l': option_flags |= 0x20;         /* Print only N lines */
                if (optarg != 0) {
                  max_num_lines = optarg[0] == '-' && optarg[1] == '\0'
                                ? -max_num_lines
                                : atoi(optarg);
                } break;
      case 'n': option_flags |= 0x1;  break;  /* Number lines */
      case 'p': option_flags |= 0x80;         /* Page */
                max_num_lines = optarg == 0 
                              ? 40
                              : atoi(optarg);
                break;
      case 's': option_flags |= 0x8;  break;  /* Squeeze blanks */
      case 't': option_flags |= 0x10 | 0x40; break;  /* Same as -vT */
      case 'T': option_flags |= 0x10; break;  /* Show tabs */
      case 'v': option_flags |= 0x40; break;  /* Show nonprinting */
      case  0: return usage(0);               /* Help */
      case  1: return version(0);             /* Version */
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
    /* NB: max_num_lines is a global which in some cases is decreased in
     * print_file (it acts as "lines left" when the -l flag is set) */
    for (i = optind; i < argc && max_num_lines != 0; ++i) {
      print_file(argv[i], option_flags);
    }
  }
  if (option_flags & 0x20 && max_num_lines < 0) {
    strlst_print_all();
  }
  return 0;
}
