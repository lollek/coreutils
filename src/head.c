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
  fprintf(status ? stderr : stdout, "lollek-coreutils/head v0\n");
  return status;
}

int usage(int status) {
  /* status == 0 -> print to stdout, exit 0
   * status == 1 -> print to stderr, exit 1 */
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "Print the first 10 lines of each FILE to standard output\n"
      "If more than one FILE, each will contain a header with the filename\n\n"
      , progname);
  fprintf(status ? stderr : stdout,
      /*"  -T, --show-tabs          display TAB characters as ^I\n" */
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n"
      "With no FILE, or when FILE is -, standard input is read\n");
  return version(status);
}

int print_file(const char *filename, unsigned option_flags, int num_lines) {
  FILE *fd = NULL;
  char buf[BUFLEN];

  /* Open */
  if (!strcmp(filename, "-")) {
    fd = stdin;
  } else {
    if ((fd = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
      return 1;
    }
  }

  /* Header */
  if (option_flags & 0x1) {
    static bool first_file = true;
    printf("%s==> %s <==\n", first_file ? "" : "\n", filename);
    first_file = false;
  }

  /* Read */
  while (fgets(buf, BUFLEN, fd) != NULL) {
    char *c;
    for (c = buf; *c != '\0'; ++c) {
      if (*c == '\n' && --num_lines < 0) {
        fclose(fd);
        return 0;
      }
    }
    printf("%s", buf);
  }

  /* Close */
  fclose(fd);
  return 0;
  (void)option_flags;
}

int main(int argc, char **argv) {
  int option_index = 0;
  int num_lines = 10;
  unsigned option_flags = 0;
  /* OPTION_FLAGS
   * 0x1 - Print header? */

  struct option long_options[] = {
    {"help",            no_argument, 0,  0},
    {"version",         no_argument, 0,  1}
  };
  progname = argv[0];

  while (1) {
    int c = getopt_long(argc, argv, "", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case  0: return usage(0);              /* Help */
      case  1: return version(0);            /* Version */
      default:
        fprintf(stderr, "Try '%s --help' for more information\n",
                progname);
        return 1;
    }
  }

  if (optind == argc) {
    print_file("-", option_flags, num_lines);
  } else {
    int i;
    if (argc - optind > 1) {
      option_flags |= 0x1;
    }
    for (i = optind; i < argc; ++i) {
      print_file(argv[i], option_flags, num_lines);
    }
  }
  return 0;
}
