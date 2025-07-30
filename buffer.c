#include "buffer.h"

#include <string.h>

#include "vec.h"

void buffer_init(buffer_t* b) { b->data = NULL; }

void buffer_free(buffer_t* b) { vec_free(b->data); }

void buffer_grow(buffer_t* b, size_t n) {
    vec_reserve(&b->data, vec_len(b->data) + n);
}

size_t buffer_len(const buffer_t* b) { return vec_len(b->data); }

void buffer_write(buffer_t* b, const void* data, size_t len) {
    const size_t end = vec_len(b->data);
    vec_resize(&b->data, end + len);
    memcpy(&b->data[end], data, len);
}

void buffer_write_char(buffer_t* b, char c) { buffer_write(b, &c, 1); }
