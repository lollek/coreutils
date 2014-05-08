#include <stdio.h>
#include <stdlib.h>       /* malloc, realloc, free, abort */
#include <stdbool.h>
#include <string.h>       /* strerror, strcpy, strcmp */
#include <sys/stat.h>     /* stat */
#include <dirent.h>       /* opendir, readdir, closedir */
#include <errno.h>        /* errno */
#include <linux/limits.h> /* NAME_MAX */

#include "ls.h"

extern const char *progname; 
extern ls_sort_t sorting;
extern list_which_files_t list_which_files;

int version(int status) {
  fprintf(status ? stderr : stdout, "lollek-coreutils/ls v0.2\n");
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

void sort_pathv(int pathc, const char **pathv, struct stat **statv) {
  int i, j;
  void (*sorting_fun)(int, int, const char **, struct stat **);

  sorting_fun = statv != NULL
    ? sort_pathv_filetype_coll
    : sort_pathv_coll;

  for (i = 0; i < pathc; ++i) {
    for (j = 1; j < pathc - i; ++j) {
      sorting_fun(j-1, j, pathv, statv);
    }
  }
}

void sort_pathv_coll(int lh, int rh, const char **pathv,
                     struct stat **statv __attribute__ ((unused))) {
  if (strcmp(pathv[lh], pathv[rh]) > 0) {
    const char *pathptr = pathv[rh];
    pathv[rh] = pathv[lh];
    pathv[lh] = pathptr;
  }
}

void sort_pathv_filetype_coll(int lh, int rh, const char **pathv,
                              struct stat **statv) {
  bool lh_is_dir;
  bool rh_is_dir;

  if (statv[lh] == NULL) {
    return;
  }

  if (statv[rh] != NULL) {
    lh_is_dir = S_ISDIR(statv[lh]->st_mode);
    rh_is_dir = S_ISDIR(statv[rh]->st_mode);
  }

  if (statv[rh] == NULL ||
      (lh_is_dir && !rh_is_dir) ||
      (lh_is_dir == rh_is_dir && strcmp(pathv[lh], pathv[rh]) > 0)) {
    struct stat *statptr = statv[rh];
    const char *pathptr = pathv[rh];

    statv[rh] = statv[lh];
    statv[lh] = statptr;

    pathv[rh] = pathv[lh];
    pathv[lh] = pathptr;
  }
}

int print_path(const char *path, struct stat *pathstat) {
  bool free_pathstat = pathstat == NULL ? true : false;
  if (pathstat == NULL) {
    pathstat = xmalloc(sizeof *pathstat);
    if (stat(path, pathstat) == -1) {
      fprintf(stderr, "%s: cannot access %s: %s\n",
              progname, path, strerror(errno));
      free(pathstat);
      return 1;
    }
  }

  if (S_ISDIR(pathstat->st_mode)) {
    int i;
    int dirc = 0;
    int max_dirc = 256;
    char **dirv = xmalloc(sizeof *dirv * max_dirc);
    DIR *pathdir = opendir(path);
    struct dirent *path_dirent = NULL;

    if (pathdir == NULL) {
      fprintf(stderr, "%s: cannot access %s: %s\n",
              progname, path, strerror(errno));
      return 1;
    }

    while ((path_dirent = readdir(pathdir)) != NULL) {
      if (dirc == max_dirc) {
        max_dirc *= 2;
        dirv = xrealloc(dirv, sizeof *dirv * max_dirc);
      }
      switch (list_which_files) {
        case ALL: break;
        case ALMOST_ALL: if (path_dirent->d_name[0] == '.' &&
                             (path_dirent->d_name[1] == '\0' ||
                              (path_dirent->d_name[1] == '.' &&
                               path_dirent->d_name[2] == '\0'))) {
                           continue;
                         } break;
        case NOT_HIDDEN: if (path_dirent->d_name[0] == '.') {
                           continue;
                         } break;
      }
      dirv[dirc] = xmalloc(sizeof *dirv[dirc] * (NAME_MAX +1));
      strcpy(dirv[dirc], path_dirent->d_name);
      ++dirc;
    }

    sort_pathv(dirc, (const char **) dirv, NULL);
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
  } else {
    printf("%s ", path);
  }

  if (free_pathstat) {
    free(pathstat);
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
      }
      print_path(pathv[i], statv[i]);
    }
  }
  if (printing_files) {
    putchar('\n');
  }
  free_statv(pathc, statv);
  return 0;
}

