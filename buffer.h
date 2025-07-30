#ifndef CLAIRE_BUFFER_H
#define CLAIRE_BUFFER_H

#include <stddef.h>

typedef struct {
    char* data;
} buffer_t;

void buffer_init(buffer_t* b);

void buffer_free(buffer_t* b);

void buffer_grow(buffer_t* b, size_t n);

size_t buffer_len(const buffer_t* b);

void buffer_write(buffer_t* b, const void* data, size_t len);

void buffer_write_char(buffer_t* b, char c);

#endif
