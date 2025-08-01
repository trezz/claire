#include "map.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_abc(void) {
    const char want[26] = "abcdefghijklmnopqrstuvwxyz";
    char got[26] = {0};
    char k = 'a';
    int i = 0;
    Map* m = mapNew(sizeof(char), 0);

    while (k <= 'z') {
        mapSet(m, &k, 1, &k);
        ++k;
    }

    for (i = 0; i < 26; ++i) {
        char k = 'a' + i;
        char v = 0;
        if (!mapGet(m, &k, 1, &v)) {
            printf("Key '%c' not found in map\n", k);
            mapFree(m);
            return 1;
        }
        got[i] = v;
    }
    if (strncmp(want, got, 26) != 0) {
        printf("Map values mismatch: want '%s', got '%s'\n", want, got);
        mapFree(m);
        return 1;
    }

    mapFree(m);
    return 0;
}

static int test_keys(void) {
    /* Got using `cat ./map/testdata/keys | sort -u | wc -l` */
    const size_t want_unique_keys_count = 573697;
    /* Got using `cat ./map/testdata/keys | wc -l` */
    const size_t want_keys_count = 1623420;

    FILE* f = fopen("./testdata/keys", "r");
    char line[1024];
    Map* keys_count = mapNew(sizeof(size_t), 0);
    size_t count = 0;
    MapIt it = {0};

    while (fgets(line, sizeof(line), f) != NULL) {
        const size_t len = strlen(line);
        size_t* c = mapAt(keys_count, line, len);
        if (c) {
            ++(*c);
        } else {
            const size_t v = 1;
            mapSet(keys_count, line, len, &v);
        }
    }
    fclose(f);

    if (mapLen(keys_count) != want_unique_keys_count) {
        printf("Unique keys count mismatch: want %zu, got %zu\n",
               want_unique_keys_count, mapLen(keys_count));
        mapFree(keys_count);
        return 1;
    }

    while (mapIter(keys_count, &it)) {
        count += *(size_t*)it.value;
    }

    if (count != want_keys_count) {
        printf("Keys count mismatch: want %zu, got %zu\n", want_keys_count,
               count);
        mapFree(keys_count);
        return 1;
    }

    mapFree(keys_count);
    return 0;
}

int test_intset(void) {
    size_t keys[100];
    Map* m = mapNew(0, 0);
    int i = 0;
    size_t present[100] = {0};
    size_t want_count = 0;

    for (i = 0; i < 100; ++i) {
        keys[i] = rand() % 100;
        present[keys[i]] = 1;
    }

    for (i = 0; i < 100; ++i) {
        if (present[i]) {
            ++want_count;
        }
    }
    for (i = 0; i < 100; ++i) {
        mapSet(m, &keys[i], sizeof(size_t), NULL);
    }

    if (mapLen(m) != want_count) {
        printf("Map length mismatch: want %zu, got %zu\n", want_count,
               mapLen(m));
        mapFree(m);
        return 1;
    }

    mapFree(m);
    return 0;
}

int main(void) {
    if (test_abc() != 0) {
        return 1;
    }
    if (test_keys() != 0) {
        return 1;
    }
    if (test_intset() != 0) {
        return 1;
    }
    return 0;
}
