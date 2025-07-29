#include "map.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    const char want[26] = "abcdefghijklmnopqrstuvwxyz";
    char got[26] = {0};
    char k = 'a';
    int i = 0;

    map_t m = map_new(sizeof(char), 0);

    while (k <= 'z') {
        map_add(m, &k, 1, k);
        ++k;
    }

    for (i = 0; i < 26; ++i) {
        char k = 'a' + i;
        char v = 0;
        if (!map_get(m, &k, 1, &v)) {
            printf("Key '%c' not found in map\n", k);
            return 1;
        }
        got[i] = v;
    }
    if (strncmp(want, got, 26) != 0) {
        printf("Map values mismatch: want '%s', got '%s'\n", want, got);
        return 1;
    }
}
