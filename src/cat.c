#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#define BUFLEN 256

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

/* Shift the first line in list */
void strlst_shift() {
  if (strlst_end != NULL) {
    struct strlst *node = NULL;
    while (!strlst_start->has_newline) {
      node = strlst_start;
      strlst_start = strlst_start->next;
      free(node);
    }
    node = strlst_start;
    strlst_start = strlst_start->next;
    free(node);
  }
}

/* Push a new line to list */
int strlst_push(const char *string, int num_lines) {
  const char *strptr = string;
  int id;

  /* Initialize the list if needed */
  if (strlst_end == NULL) {
    if ((strlst_start = (struct strlst *)malloc(sizeof(struct strlst)))
        == NULL) {
      perror("malloc");
      exit(1);
    }
    strlst_start->id = 0;
    strlst_start->has_newline = false;
    strlst_start->data[0] = '\0';
    strlst_start->next = NULL;
    strlst_end = strlst_start;
  }

  /* Shift first line in queue if we've reached the max */
  if (strlst_end->id == num_lines) {
    strlst_shift();
  }

  /* Add chunks of BUFLEN sized strings */
  while (strlen(strptr) > BUFLEN -1) {
    id = strlst_end->id;
    if ((strlst_end->next = (struct strlst *)malloc(sizeof(struct strlst)))
        == NULL) {
      perror("malloc");
      exit(1);
    }
    strlst_end = strlst_end->next;
    strlst_end->id = id;
    strlst_end->has_newline = false;
    strncpy(strlst_end->data, strptr, BUFLEN -1);
    strlst_end->data[BUFLEN -1] = '\0';
    strlst_end->next = NULL;
    strptr += BUFLEN -1;
  }
  /* Add last string, which usually has newline */
  id = strlst_end->id;
  if ((strlst_end->next = (struct strlst *)malloc(sizeof(struct strlst)))
      == NULL) {
    perror("malloc");
    exit(1);
  }
  strlst_end = strlst_end->next;
  strlst_end->id = id;
  strlst_end->has_newline = false;
  strncpy(strlst_end->data, strptr, BUFLEN -1);
  strlst_end->data[BUFLEN -1] = '\0';
  strlst_end->next = NULL;

  /* Check if it has newline and return */
  for(strptr = strlst_end->data; *strptr != '\0'; ++strptr) {
    if (*strptr == '\n') {
      if (strlst_end->id != num_lines) {
        ++strlst_end->id;
      }
      strlst_end->has_newline = true;
      return 0;
    }
  }
  return 0;
}

void strlst_print_all() {
  struct strlst *node = NULL;
  while (strlst_start != NULL) {
    node = strlst_start;
    strlst_start = strlst_start->next;
    printf("%s", node->data);
    free(node);
  }
}


/** End of linked list & friends **/


const char *progname = NULL;

int version(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout, "lollek-coreutils/cat v1.1\n");
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
      "  -b, --number-nonblank    number nonempty output lines, overrides -n\n"
      "  -E, --show-ends          display $ at end of each line\n"
      "  -l, --lines=[-]N         print only the first/last N lines (default 10)\n");
  fprintf(status ? stderr : stdout,
      "  -n, --number             number all output lines\n"
      "  -s, --squeeze-blank      suppress repeated empty output lines\n"
      "  -T, --show-tabs          display TAB characters as ^I\n"
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n"
      "With no FILE, or when FILE is -, standard input is read\n");
  return version(status);
}

int print_file(const char *filename, unsigned option_flags, int *num_lines) {
  static unsigned line_counter = 0;
  static bool last_line_was_blank = false;
  static bool prevent_enumeration = false;
  FILE *fd = NULL;
  char buf[BUFLEN];
  char tmpbuf[BUFLEN];
  tmpbuf[BUFLEN -1] = '\0';

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
        if (*num_lines > 0) {
          printf("%6d\t", ++line_counter);
        } else {
          snprintf(tmpbuf, BUFLEN -1, "%6d\t", ++line_counter);
          strlst_push(tmpbuf, -*num_lines);
        }
      }

      /* Show tabs */
      if (option_flags & 0x10) {
        while (*c != '\0' && *c != '\n') {
          if (*c == '\t') {
            *c = '\0';
            if (*num_lines > 0) {
              printf("%s^I", bufptr);
            } else {
              int chars_to_print = strlen(bufptr) + strlen("^I");
              while (chars_to_print > 0) {
                chars_to_print -= snprintf(tmpbuf, BUFLEN -1, "%s^I", bufptr);
                strlst_push(tmpbuf, -*num_lines);
              }
            }
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
          if (*num_lines > 0) {
            printf("%s$\n", bufptr);
          } else {
            int chars_to_print = strlen(bufptr) + strlen("$\n");
            while (chars_to_print > 0) {
              chars_to_print -= snprintf(tmpbuf, BUFLEN -1, "%s$\n", bufptr);
              strlst_push(tmpbuf, -*num_lines);
            }
          }
        } else {
          if (*num_lines > 0) {
            printf("%s\n", bufptr);
          } else {
            int chars_to_print = strlen(bufptr) + strlen("\n");
            while (chars_to_print > 0) {
              chars_to_print -= snprintf(tmpbuf, BUFLEN -1, "%s\n", bufptr);
              strlst_push(tmpbuf, -*num_lines);
            }
          }
        }
        /* If we only print N lines */
        if (option_flags & 0x20) {
          if (*num_lines > 0 && --*num_lines == 0) {
            break;
          }
        }
        prevent_enumeration = false;
      } else {
        if (*num_lines > 0) {
          printf("%s", bufptr);
        } else {
          int chars_to_print = strlen(bufptr);
          while (chars_to_print > 0) {
            chars_to_print -= snprintf(tmpbuf, BUFLEN, "%s", bufptr);
            strlst_push(tmpbuf, -*num_lines);
          }
        }
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
  int num_lines = 10;
  unsigned option_flags = 0;
  int option_index = 0;
  struct option long_options[] = {
    {"number-nonblank", no_argument,       0, 'b'},
    {"show-ends",       no_argument,       0, 'E'},
    {"lines",           optional_argument, 0, 'l'},
    {"number",          no_argument,       0, 'n'},
    {"squeeze-blank",   no_argument,       0, 's'},
    {"show-tabs",       no_argument,       0, 'T'},
    {"help",            no_argument,       0,  0 },
    {"version",         no_argument,       0,  1 },
    {0,                 0,                 0,  0 }
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "bEl::nsT", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'b': option_flags |= 0x4;  break;  /* Number nonblank */
      case 'E': option_flags |= 0x2;  break;  /* Show ends */
      case 'l': option_flags |= 0x20;         /* Number lines */
                if (optarg != 0) {
                  num_lines = optarg[0] == '-' && optarg[1] == '\0'
                            ? -num_lines
                            : atoi(optarg);
                } break;
      case 'n': option_flags |= 0x1;  break;  /* Number lines */
      case 's': option_flags |= 0x8;  break;  /* Squeeze blanks */
      case 'T': option_flags |= 0x10; break;  /* Show tabs */
      case  0: return usage(0);               /* Help */
      case  1: return version(0);             /* Version */
      default:
        fprintf(stderr, "Try '%s --help' for more information\n", 
                progname);
        return 1;
    }
  }

  if (optind == argc) {
    print_file("-", option_flags, &num_lines);
  } else {
    int i;
    for (i = optind; i < argc && num_lines != 0; ++i) {
      print_file(argv[i], option_flags, &num_lines);
    }
  }
  if (num_lines < 0) {
    strlst_print_all();
  }
  return 0;
}
