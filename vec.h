#ifndef CLAIRE_VEC_H
#define CLAIRE_VEC_H

#include <stddef.h>

#define vec_new(T, len, capacity) ((T *)_vec_new(sizeof(T), (len), (capacity)))

void *_vec_new(size_t value_size, size_t len, size_t capacity);

void vec_free(void *vec);

size_t vec_len(const void *vec);

size_t vec_capacity(const void *vec);

#define vec_resize(vec_ptr, len) \
    _vec_resize((vec_ptr), sizeof(*(vec_ptr)), (len))

void _vec_reserve(void *vec_ptr, size_t value_size, size_t capacity);

#define vec_reserve(vec_ptr, capacity) \
    _vec_reserve((vec_ptr), sizeof(*(vec_ptr)), (capacity))

void _vec_resize(void *vec_ptr, size_t value_size, size_t len);

#define CLAIRE_VEC_JOIN2(a, b, c) a##b##c
#define CLAIRE_VEC_JOIN(a, b, c) CLAIRE_VEC_JOIN2(a, b, c)
#define CLAIRE_VEC_UNIQUE(id) CLAIRE_VEC_JOIN(_claire_vec_, id, __LINE__)

#define vec_append(vec_ptr, value)                                      \
    do {                                                                \
        const size_t CLAIRE_VEC_UNIQUE(_vec_len) = vec_len(*(vec_ptr)); \
        vec_resize((vec_ptr), CLAIRE_VEC_UNIQUE(_vec_len) + 1);         \
        (*(vec_ptr))[CLAIRE_VEC_UNIQUE(_vec_len)] = (value);            \
    } while (0)

#endif
