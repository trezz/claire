#include "vec.h"

#include <stdio.h>

int main(void) {
    size_t *vec = vec_new(size_t);
    size_t i = 0;

    for (i = 0; i < 20; ++i) {
        vec_append(&vec, i);
    }

    for (i = 0; i < vec_len(vec); ++i) {
        if (vec[i] != i) {
            fprintf(stderr, "Error: vec[%zu] = %zu, expected %zu\n", i, vec[i],
                    i);
            return 1;
        }
    }

    vec_free(vec);
    return 0;
}
