#include "map.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

/*
 * Configuration constants for the map implementation.
 * These can be adjusted to optimize performance for specific use cases.
 */
#define BUCKET_CAPA 8           /* Number of key-value pairs per bucket. */
#define KEY_CAPA 1024           /* Initial capacity for the keys buffer. */
#define MAP_MAX_LOAD_FACTOR 6.5 /* Maximum load factor before rehashing. */
#define MAX_VALUE_SIZE sizeof(int64_t) /* Maximum value size supported. */

typedef struct bucket {
    size_t hashes[BUCKET_CAPA];
    size_t keys[BUCKET_CAPA];
    char values[BUCKET_CAPA * MAX_VALUE_SIZE];
    size_t len;
    struct bucket* next;
} bucket_s;

typedef struct map {
    int seed;
    size_t key_size;
    size_t value_len;
    size_t capacity;
    size_t len;

    bucket_s* buckets;
    size_t buckets_len;

    char* keys;
    size_t keys_len;
    size_t keys_cap;
} map_s;

#define KEY_SIZE_MIXED SIZE_MAX
#define KEY_SIZE_UNKNOWN 0

static void init_map(map_s* m, size_t value_len, size_t capacity) {
    m->seed = rand();
    m->key_size = KEY_SIZE_UNKNOWN;
    m->value_len = value_len;
    m->capacity = capacity == 0 ? BUCKET_CAPA : capacity;
    while (m->capacity % BUCKET_CAPA != 0) {
        ++m->capacity;
    }
    m->len = 0;

    m->buckets_len = m->capacity / BUCKET_CAPA;
    m->buckets = malloc(m->buckets_len * sizeof(bucket_s));
    memset(m->buckets, 0, m->buckets_len * sizeof(bucket_s));

    m->keys = malloc(KEY_CAPA);
    assert(m->keys != NULL && "Failed to allocate memory for keys buffer");
    m->keys_len = 0;
    m->keys_cap = KEY_CAPA;
}

static void deinit_map(map_s* m) {
    size_t i = 0;
    for (i = 0; i < m->buckets_len; ++i) {
        bucket_s* b = &m->buckets[i];
        while (b->next != NULL) {
            bucket_s* next = b->next->next;
            free(b->next);
            b->next = next;
        }
    }
    free(m->buckets);
    free(m->keys);
}

Map* mapNew(size_t valueLen, size_t capacity) {
    map_s* m = NULL;

    assert(valueLen <= MAX_VALUE_SIZE &&
           "Value size must not exceed the maximum supported size");

    m = malloc(sizeof(map_s));
    assert(m != NULL && "Failed to allocate memory for map");
    init_map(m, valueLen, capacity);
    return m;
}

void mapFree(Map* m) {
    map_s* map = m;
    deinit_map(map);
    free(map);
}

size_t mapLen(const Map* m) {
    const map_s* map = m;
    return map->len;
}

#define bucket_pos(m, h) ((h) & ((m)->buckets_len - 1))
#define bucket_val(m, b, i) ((b)->values + ((i) * (m)->value_len))

typedef struct {
    size_t hash;
    const void* data;
    size_t len;
} key_s;

static key_s make_key(const map_s* m, const void* data, size_t len) {
    key_s k;
    k.hash = hash_bytes(data, len, m->seed);
    k.data = data;
    k.len = len;
    return k;
}

/*
 * Element in the map identified by its key which holds its bucket, its position
 * in the bucket and whether the element already exists in the map.
 */
typedef struct {
    bucket_s* bucket; /* Pointer to the bucket that should hold the key. */
    size_t pos;       /* Position in the bucket for the key. */
    int found;        /* Non-zero if the key already exists in the map. */
} elem_s;

/*
 * Finds the bucket and the position within the bucket for the given key.
 */
static elem_s find_key(const map_s* m, const key_s* k) {
    const size_t bpos = bucket_pos(m, k->hash);
    bucket_s* b = &m->buckets[bpos];
    size_t i = 0;
    elem_s res = {0};

    while (1) {
        for (i = 0; i < b->len; ++i) {
            const char* bkey = NULL;
            if (k->hash != b->hashes[i]) {
                continue;
            }
            bkey = m->keys + b->keys[i];
            if (!strncmp(k->data, bkey, k->len)) {
                break;
            }
        }
        if (i < b->len) {
            break;
        }
        if (b->next == NULL) {
            break;
        }
        b = b->next;
    }

    res.bucket = b;
    res.pos = i;
    res.found = i < b->len;
    return res;
}

int mapGet(const Map* m, const void* key, size_t keyLen, void* dest) {
    const map_s* map = m;
    const void* v = mapAt(m, key, keyLen);
    if (v == NULL) {
        return 0;
    }
    if (dest != NULL) {
        memcpy(dest, v, map->value_len);
    }
    return 1;
}

/*
 * Insert a new key/value pair in the map and return a pointer to the map.
 */
static void insert(map_s* m, const key_s* k, const void* v) {
    elem_s elem = find_key(m, k);
    bucket_s* b = elem.bucket;

    if (elem.found) {
        if (v != NULL) {
            memcpy(bucket_val(m, b, elem.pos), v, m->value_len);
        }
        return;
    }

    if (elem.pos == BUCKET_CAPA) {
        b->next = malloc(sizeof(bucket_s));
        assert(b->next != NULL && "Failed to allocate memory for new bucket");
        memset(b->next, 0, sizeof(bucket_s));
        b = b->next;
        elem.pos = 0;
    }
    if (v != NULL) {
        memcpy(bucket_val(m, b, elem.pos), v, m->value_len);
    }
    b->hashes[elem.pos] = k->hash;

    if (m->keys_len + k->len + 1 > m->keys_cap) {
        m->keys_cap = m->keys_cap * 2;
        m->keys = realloc(m->keys, m->keys_cap);
        assert(m->keys != NULL &&
               "Failed to reallocate memory for keys buffer");
    }
    b->keys[elem.pos] = m->keys_len;
    memcpy(m->keys + m->keys_len, k->data, k->len);
    m->keys_len += k->len;
    m->keys[m->keys_len++] = 0;

    if (m->key_size == KEY_SIZE_UNKNOWN) {
        m->key_size = k->len;
    } else if (m->key_size != k->len) {
        m->key_size = KEY_SIZE_MIXED;
    }

    ++b->len;
    ++m->len;
}

/*
 * Increase the map capacity by 2 and rehashs the existing key/value pairs.
 * A pointer to the rehased map is returned.
 * NULL is returned in case of error.
 */
static void rehash(map_s* m) {
    map_s tmp;
    MapIt it = {0};
    init_map(&tmp, m->value_len, m->capacity * 2);
    while (mapIter(m, &it)) {
        key_s k = make_key(&tmp, it.key, it.keyLen);
        insert(&tmp, &k, it.value);
    }
    deinit_map(m);
    memcpy(m, &tmp, sizeof(map_s));
}

void mapSet(Map* m, const void* key, size_t keyLen, const void* value) {
    map_s* map = m;
    const double load_factor = (double)(map->len) / (double)map->buckets_len;
    key_s k;

    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        rehash(map);
    }
    k = make_key(map, key, keyLen);
    insert(map, &k, value);
}

void* mapAt(const Map* m, const void* key, size_t keyLen) {
    const map_s* map = m;
    const key_s k = make_key(m, key, keyLen);
    const elem_s elem = find_key(m, &k);
    return elem.found ? bucket_val(map, elem.bucket, elem.pos) : NULL;
}

int mapDelete(Map* m, const void* key, size_t keyLen) {
    map_s* map = m;
    const key_s k = make_key(m, key, keyLen);
    const elem_s elem = find_key(m, &k);

    if (!elem.found) {
        return 0;
    }

    if (elem.bucket->len > 1) {
        const size_t last = elem.bucket->len - 1;
        elem.bucket->hashes[elem.pos] = elem.bucket->hashes[last];
        elem.bucket->keys[elem.pos] = elem.bucket->keys[last];
        memcpy(bucket_val(map, elem.bucket, elem.pos),
               bucket_val(map, elem.bucket, last), map->value_len);
    }
    --elem.bucket->len;
    --map->len;
    return 1;
}

int mapIter(const Map* m, MapIt* it) {
    const map_s* map = m;
    const size_t nb_buckets = map->buckets_len;

    if (map->len == 0) {
        return 0;
    }

    if (it->_b == NULL) {
        it->_b = &map->buckets[0];
    }

    while (it->_bpos < nb_buckets) {
        bucket_s* b = it->_b;
        for (; b != NULL; it->_b = b = b->next, it->_kpos = 0) {
            if (it->_kpos >= b->len) {
                continue;
            }
            it->key = map->keys + b->keys[it->_kpos];
            it->keyLen = (map->key_size == KEY_SIZE_MIXED ||
                          map->key_size == KEY_SIZE_UNKNOWN)
                             ? (size_t)strlen(it->key)
                             : (size_t)map->key_size;
            it->value = bucket_val(map, b, it->_kpos);
            ++it->_kpos;
            return 1;
        }
        it->_kpos = 0;
        if (++it->_bpos == nb_buckets) {
            return 0;
        }
        it->_b = &map->buckets[it->_bpos];
    }
    return 0;
}
