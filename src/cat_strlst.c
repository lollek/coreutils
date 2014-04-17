#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "cat_strlst.h"

/* Globals */
static strlst *strlst_start = NULL;
static strlst *strlst_end = NULL;
static int nodes_in_list = 0;

int strlst_num_nodes() {
  return nodes_in_list;
}

/* Prints and removes all lines from strlst */
void strlst_flush() {
  strlst *delptr = strlst_start;
  while (delptr != NULL) {
    strlst_start = strlst_start->next;
    printf("%s", delptr->data);
    free(delptr);
    delptr = strlst_start;
  }
  strlst_end = NULL;
  nodes_in_list = 0;
}


/* Remove the first line in the list */
void strlst_shift() {
  strlst *delptr = strlst_start;
  if (strlst_end == NULL) {
    return;
  }

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

  --nodes_in_list;
}

/* Push a new line to list 
 * string should never be larger than BUFLEN in size (including '\0') */
void strlst_push(const char *string) {
  bool has_newline = false;
  const char *strptr = string;

  /* Check for newlines */
  for (; *strptr != '\0'; ++strptr) {
    if (*strptr == '\n') {
      has_newline = true;
      break;
    }
  }
  strptr = string;

  /* If neither it nor the previous string does, we can try to concat them.*/
  if (!has_newline && strlst_end != NULL && !strlst_end->has_newline &&
      strlen(string) + strlen(strlst_end->data) < BUFLEN) {
    strcat(strlst_end->data, string);
    return;
  }

  /* Otherwise, we add it as a new node */
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
  strlst_end->has_newline = has_newline;
  strncpy(strlst_end->data, strptr, BUFLEN);
  strlst_end->next = NULL;

  ++nodes_in_list;
}
