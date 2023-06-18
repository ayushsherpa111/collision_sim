#include "./vector.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


vector *make(size_t _elem_size, int cap) {
  vector *new_vec = malloc(sizeof(vector));
  new_vec->elements = malloc(_elem_size * cap);
  new_vec->len = 0;
  new_vec->cap = cap;
  return new_vec;
}

void append(vector *source, void *element) {
  if ((source->len + 1) > source->cap) {
    void *new_block = realloc(source->elements, source->cap * 2);
    source->elements = new_block;
    source->cap = source->cap * 2;
  }
  source->elements[source->len] = element;
  source->len += 1;
}

void* pop(vector *source) {
  void* popped = source->elements[0];
  source->len--;
  void **temp = malloc(source->elem_size * source->cap);
  memcpy(temp, source->elements+1, source->elem_size * source->cap);
  free(source->elements);
  source->elements = temp;
  return popped;
}

void *get(vector *src, int index) {
  if (index > src->len) {
    return NULL;
  }
  return src->elements[index];
}
