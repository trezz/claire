#include "hash.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char* data;
    size_t want_hash;
} tc_s;

int main(void) {
    const tc_s testCases[] = {{"Hello, world!", 11600739918808951577u},
                              {"Another test", 3559310193689980990},
                              {"Yet another test", 13188486659528907044u}};
    const size_t testCasesLen = sizeof(testCases) / sizeof(tc_s);

    size_t i = 0;
    for (i = 0; i < testCasesLen; i++) {
        const size_t hash =
            hashBytes(testCases[i].data, strlen(testCases[i].data), 0);
        if (hash != testCases[i].want_hash) {
            printf("Hash mismatch for input '%s': got %zu, want %zu\n",
                   testCases[i].data, hash, testCases[i].want_hash);
            return 1;
        }
    }

    return 0;
}
