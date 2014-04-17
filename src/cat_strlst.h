#ifndef __LOLLEK_CAT_STRLST_H__
#define __LOLLEK_CAT_STRLST_H__

#ifndef BUFLEN
#define BUFLEN 256
#endif

/* Linked list of chars buffers 
   These are used to 'tail' or page a file */
typedef struct strlst {
  bool has_newline;
  char data[BUFLEN];
  struct strlst *next;
} strlst;

/* Returns the number of lines in strlst */
int strlst_num_nodes();

/* Prints and removes all lines from strlst */
void strlst_flush();

/* Pushes string to strlst */
void strlst_push(const char *string);

/* Removes line from strlst */
void strlst_shift();

#endif /*__LOLLEK_CAT_STRLST_H__ */
