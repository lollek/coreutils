#ifndef __LOLLEK_LS_H__
#define __LOLLEK_LS_H__

typedef enum ls_sort_t {
  COLL
} ls_sort_t;

/** version: prints version and returns
 ** usage: prints usage and version and returns 
 *
 * Status 0 means success, in which case we use stdin and returns 0
 * Otherwise, we use stderr and return status
 * */
int version(int status);
int usage(int status);

/* These are like the stdlib.h functions, but aborts on error */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t newsize);

/** xmalloc_statv: Takes a list of paths and creates a stat for each
 ** free_statv: Free's the result of xmalloc_statv 
 *
 * pathc is the number of elements in pathv
 * pathv is a list of paths
 * statv is the result of xmalloc_statv
 * */
struct stat **xmalloc_statv(int pathc, const char **pathv);
void free_statv(int pathc, struct stat **statv);

/* Sorts pathv (and statv is it exists) 
 * pathc is the number of elements in pathv
 * pathv is a list of paths
 * statv is a list of stat for the path of the same index
 * lh is the index of the left element to be compared
 * rh is the index of the right element to be compared
 * */
void sort_pathv(int pathc, const char **pathv, struct stat **statv);
void sort_pathv_coll(int lh, int rh, const char **pathv,
                     struct stat **statv __attribute__ ((unused)));
void sort_pathv_filetype_coll(int lh, int rh, const char **pathv,
                              struct stat **statv);

int print_path(const char *path);
int ls(int pathc, const char **pathv);

#endif /* __LOLLEK_LS_H__ */
