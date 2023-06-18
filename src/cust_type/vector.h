#pragma once

#include <stddef.h>


#define VECTOR_GET(vec, type, idx) *(type*) get(vec, idx)
typedef struct vector vector;
struct vector {
  void **elements;
  int len;
  int cap;
  size_t elem_size;
};

vector* make(size_t elem_size, int cap);
void append(vector *vec, void *element);
void* pop(vector *src);
void* get(vector *src, int index);
