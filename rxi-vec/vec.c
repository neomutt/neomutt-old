/** 
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include "vec.h"
#include "mutt/memory.h"


void vec_expand_(char **data, int *length, int *capacity, int memsz) {
  if (*length + 1 > *capacity) {
    int n = (*capacity == 0) ? 1 : *capacity << 1;
    mutt_mem_realloc(data, n * memsz);
    *capacity = n;
  }
}


void vec_reserve_(char **data, int *length, int *capacity, int memsz, int n) {
  (void) length;
  if (n > *capacity) {
    mutt_mem_realloc(data, n * memsz);
    *capacity = n;
  }
}


void vec_reserve_po2_(char **data, int *length, int *capacity, int memsz, int n) {
  int n2 = 1;
  if (n == 0) return;
  while (n2 < n) n2 <<= 1;
  vec_reserve_(data, length, capacity, memsz, n2);
}


void vec_compact_(char **data, int *length, int *capacity, int memsz) {
  if (*length == 0) {
    mutt_mem_free(*data);
    *data = NULL;
    *capacity = 0;
  } else {
    int n = *length;
    mutt_mem_realloc(data, n * memsz);
    *capacity = n;
  }
}


void vec_insert_(char **data, int *length, int *capacity, int memsz,
                 int idx
) {
  vec_expand_(data, length, capacity, memsz);
  memmove(*data + (idx + 1) * memsz,
          *data + idx * memsz,
          (*length - idx) * memsz);
}


void vec_splice_(char **data, int *length, int *capacity, int memsz,
                 int start, int count
) {
  (void) capacity;
  memmove(*data + start * memsz,
          *data + (start + count) * memsz,
          (*length - start - count) * memsz);
}


void vec_swapsplice_(char **data, int *length, int *capacity, int memsz,
                     int start, int count
) {
  (void) capacity;
  memmove(*data + start * memsz,
          *data + (*length - count) * memsz,
          count * memsz);
}


void vec_swap_(char **data, int *length, int *capacity, int memsz,
               int idx1, int idx2 
) {
  unsigned char *a, *b, tmp;
  int count;
  (void) length;
  (void) capacity;
  if (idx1 == idx2) return;
  a = (unsigned char*) *data + idx1 * memsz;
  b = (unsigned char*) *data + idx2 * memsz;
  count = memsz;
  while (count--) {
    tmp = *a;
    *a = *b;
    *b = tmp;
    a++, b++;
  }
}

