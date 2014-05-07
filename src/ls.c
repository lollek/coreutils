#include <stdio.h>
#include <stdlib.h> //malloc, realloc, free, abort
#include <stdbool.h>
#include <string.h> //strerror, strcpy, strcmp
#include <sys/stat.h> //stat
#include <dirent.h> //opendir, readdir, closedir
#include <getopt.h> //getopt_long
#include <errno.h> //errno
#include <linux/limits.h> //NAME_MAX

static const char *progname = NULL;
static enum ls_sort_t {
  COLL
} sorting = COLL;

int version(int status) {
  fprintf(status ? stderr : stdout, "lollek-coreutils/ls v0.1\n");
  return status;
}

int usage(int status) {
  fprintf(status ? stderr : stdout,
      "Usage: %s [OPTIONS] [FILE]\n"
      "List information about FILE (the current directory by default)\n"
      "The entires are sorted alphabetically by default\n\n",
      progname);
  fprintf(status ? stderr : stdout,
      "      --help               display this help and exit\n"
      "      --version            output version information and exit\n\n");
  return version(status);
}

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "%s: Virtual memory exhausted\n", progname);
    abort();
  }
  return ptr;
}

void *xrealloc(void *ptr, size_t newsize) {
  ptr = realloc(ptr, newsize);
  if (ptr == NULL && newsize != 0) {
    fprintf(stderr, "%s: Virtual memory exhausted\n", progname);
    abort();
  }
  return ptr;
}

struct stat **xmalloc_statv(int pathc, const char **pathv) {
  int i;

  struct stat **statv = xmalloc(sizeof *statv * pathc);

  for (i = 0; i < pathc; ++i) {
    statv[i] = xmalloc(sizeof *statv[i]);
    if (stat(pathv[i], statv[i]) == -1) {
      fprintf(stderr, "%s: cannot access %s: %s\n",
              progname, pathv[i], strerror(errno));
      free(statv[i]);
      statv[i] = NULL;
      pathv[i] = NULL;
    }
  }
  return statv;
}

void free_statv(int pathc, struct stat **statv) {
  int i;
  for(i = 0; i < pathc; ++i) {
    free(statv[i]);
  }
  free(statv);
}

/* Sorts files and folders in pathv
 * pathc: number of files and folders to sort
 * pathv: names of files and folders to sort
 * pathv: stats of files and folders to sort
 */
void sort_pathv(int pathc, const char **pathv, struct stat **statv) {
  int i, j;
  bool lh_is_dir = false;
  bool rh_is_dir = false;

  if (sorting == COLL) {
    struct stat *statptr = NULL;
    const char *pathptr = NULL;
    for (i = 0; i < pathc; ++i) {
      for (j = 1; j < pathc - i; ++j) {
        if (statv[j-1] == NULL) {
          continue;
        }
        if (statv[j] != NULL) {
          lh_is_dir = S_ISDIR(statv[j-1]->st_mode);
          rh_is_dir = S_ISDIR(statv[j]->st_mode);
        }
        if (statv[j] == NULL ||
            (lh_is_dir && !rh_is_dir) ||
            (lh_is_dir == rh_is_dir && strcmp(pathv[j-1], pathv[j]) > 0)) {
          statptr = statv[j];
          statv[j] = statv[j-1];
          statv[j-1] = statptr;

          pathptr = pathv[j];
          pathv[j] = pathv[j-1];
          pathv[j-1] = pathptr;
        }
      }
    }
  }
}

void sort_dir(int dirc, const char **dirv) {
  int i, j;

  if (sorting == COLL) {
    const char *dirptr = NULL;
    for (i = 0; i < dirc; ++i) {
      for (j = 1; j < dirc - i; ++j) {
        if (strcmp(dirv[j-1], dirv[j]) > 0) {
          dirptr = dirv[j];
          dirv[j] = dirv[j-1];
          dirv[j-1] = dirptr;
        }
      }
    }
  }
}

int printdir(const char *path) {
  DIR *pathdir = opendir(path);
  struct dirent *path_dirent = NULL;
  int max_dirc = 256;
  int dirc = 0;
  int i;
  char **dirv = xmalloc(sizeof *dirv * max_dirc);

  if (pathdir == NULL) {
    fprintf(stderr, "%s: cannot access %s: %s\n",
            progname, path, strerror(errno));
    return 1;
  }

  while ((path_dirent = readdir(pathdir)) != NULL) {
    if (dirc == max_dirc) {
      max_dirc *= 2;
      dirv = (char **) xrealloc(dirv, sizeof(char *) * max_dirc);
    }
    dirv[dirc] = xmalloc(sizeof *dirv[dirc] * (NAME_MAX +1));
    strcpy(dirv[dirc], path_dirent->d_name);
    ++dirc;
  }

  sort_dir(dirc, (const char **) dirv);
  for (i = 0; i < dirc; ++i) {
    if (dirv[i] != NULL) {
      printf("%s ", dirv[i]);
      free(dirv[i]);
    }
  }
  printf("\b\n");
  free(dirv);

  if (closedir(pathdir) == -1) {
    fprintf(stderr, "%s: error when closing %s: %s\n",
            progname, path, strerror(errno));
  }
  return 0;
}

int ls(int pathc, const char **pathv) {
  struct stat **statv = xmalloc_statv(pathc, pathv);
  bool printing_files = true;
  int i;

  sort_pathv(pathc, pathv, statv);
  for (i = 0; i < pathc; ++i) {
    if (pathv[i] != NULL) {
      if (S_ISDIR(statv[i]->st_mode)) {
        if (printing_files) {
          putchar('\n');
          printing_files = false;
        }
        printf("\n%s:\n", pathv[i]);
        printdir(pathv[i]);
      } else {
        printf("%s ", pathv[i]);
      }
    }
  }
  if (printing_files) {
    putchar('\n');
  }
  free_statv(pathc, statv);
  return 0;
}

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
    return printdir(".");
  } else if (argc - optind == 1) {
    return printdir(argv[1]);
  } else {
    return ls(argc - optind, (const char **) &argv[optind]);
  }
}
