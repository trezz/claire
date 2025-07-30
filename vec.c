#include "vec.h"

#include <assert.h>
#include <stdlib.h>

/*
 * Configuration constants for the vector implementation.
 * These can be adjusted to optimize performance for specific use cases.
 */
#define CAPACITY 16 /* Default capacity and growth factor. */

/*
 * Header of a vector. It is positioned before the first element of the vector.
 *
 * Memory layout of a vector:
 *
 *          returned address
 *           |
 *           v
 *  +--------+----+----+-----+----+---+---+-----+---+
 *  | header | e1 | e2 | ... | eN |   |   | ... |   |
 *  +--------+----+----+-----+----+---+---+-----+---+
 *           `------ length -----'
 *           `-------------- capacity --------------'
 */
typedef struct {
    size_t value_size;
    size_t len;
    size_t cap;
} header_s;

static header_s *extend_header(header_s *h, size_t value_size,
                               size_t capacity) {
    size_t c = CAPACITY;

    if (h != NULL && h->cap >= capacity) {
        return h;
    }

    while (c < capacity) {
        c *= 2;
    }

    h = realloc(h, c * value_size + sizeof(header_s));
    assert(h != NULL && "Failed to allocate memory for vector");
    h->cap = c;
    return h;
}

/* Returns a pointer on the header of the passed vector. */
#define header(vec) (((header_s *)vec) - 1)

void *_vec_new(size_t value_size, size_t len, size_t capacity) {
    header_s *h = NULL;

    assert(len <= capacity && "Length must be less than or equal to capacity");
    assert(value_size > 0 && "Value size must be greater than 0");

    h = extend_header(NULL, value_size, capacity);
    h->value_size = value_size;
    h->len = len;
    return h + 1;
}

void vec_free(void *vec) {
    if (vec == NULL) {
        return;
    }
    free(header(vec));
}

size_t vec_len(const void *vec) {
    if (vec == NULL) {
        return 0;
    }
    return header(vec)->len;
}

size_t vec_capacity(const void *vec) {
    if (vec == NULL) {
        return 0;
    }
    return header(vec)->cap;
}

void _vec_reserve(void *vec_ptr, size_t value_size, size_t capacity) {
    void **vp = vec_ptr;
    header_s *h = NULL;

    if (*vp == NULL) {
        *vp = _vec_new(value_size, 0, capacity);
        return;
    }

    h = header(*vp);
    h = extend_header(h, h->value_size, capacity);
    *vp = h + 1;
}

void _vec_resize(void *vec_ptr, size_t value_size, size_t len) {
    void **vp = vec_ptr;
    header_s *h = NULL;

    if (*vp == NULL) {
        *vp = _vec_new(value_size, len, len);
        return;
    }

    h = header(*vp);

    if (len <= h->cap) {
        h->len = len;
        return;
    }

    h = extend_header(h, h->value_size, len);
    h->len = len;
    *vp = h + 1;
}
