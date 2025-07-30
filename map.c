#include "map.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
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

    size_t nb_buckets;
    bucket_s* buckets;

    buffer_t keys;
} map_s;

#define KEY_SIZE_MIXED SIZE_MAX
#define KEY_SIZE_UNKNOWN 0

static void init_map(map_s* m, size_t value_len, size_t capacity) {
    size_t i = 0;

    m->seed = rand();
    m->key_size = KEY_SIZE_UNKNOWN;
    m->value_len = value_len;
    m->capacity = capacity == 0 ? BUCKET_CAPA : capacity;
    while (m->capacity % BUCKET_CAPA != 0) {
        ++m->capacity;
    }
    m->len = 0;

    m->nb_buckets = m->capacity / BUCKET_CAPA;
    m->buckets = malloc(sizeof(bucket_s) * m->nb_buckets);
    assert(m->buckets != NULL && "Failed to allocate memory for map buckets");

    buffer_init(&m->keys);
    buffer_grow(&m->keys, KEY_CAPA);

    for (i = 0; i < m->nb_buckets; ++i) {
        memset(&m->buckets[i], 0, sizeof(bucket_s));
    }
}

static void deinit_map(map_s* m) {
    size_t i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        bucket_s* b = &m->buckets[i];
        while (b->next != NULL) {
            bucket_s* next = b->next->next;
            free(b->next);
            b->next = next;
        }
    }
    buffer_free(&m->keys);
    free(m->buckets);
}

map_t map_new(size_t value_len, size_t capacity) {
    map_s* m = NULL;

    assert(value_len <= MAX_VALUE_SIZE &&
           "Value size must not exceed the maximum supported size");

    m = malloc(sizeof(map_s));
    assert(m != NULL && "Failed to allocate memory for map");
    init_map(m, value_len, capacity);
    return m;
}

void map_free(map_t map) {
    map_s* m = map;
    deinit_map(m);
    free(m);
}

size_t map_len(const map_t map) {
    const map_s* m = map;
    return m->len;
}

#define bucket_pos(m, h) ((h) & ((m)->nb_buckets - 1))
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
            bkey = m->keys.data + b->keys[i];
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

int map_get(const map_t map, const void* key, size_t key_len, void* dest) {
    const map_s* m = map;
    const void* v = map_at(map, key, key_len);
    if (v == NULL) {
        return 0;
    }
    if (dest != NULL) {
        memcpy(dest, v, m->value_len);
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

    b->keys[elem.pos] = buffer_len(&m->keys);
    buffer_write(&m->keys, k->data, k->len);
    buffer_write_char(&m->keys, 0);

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
    map_it_t it = {0};
    init_map(&tmp, m->value_len, m->capacity * 2);
    while (map_iter(m, &it)) {
        key_s k = make_key(&tmp, it.key, it.key_len);
        insert(&tmp, &k, it.value);
    }
    deinit_map(m);
    memcpy(m, &tmp, sizeof(map_s));
}

void map_set(map_t map, const void* key, size_t key_len, ...) {
    map_s* m = map;
    const double load_factor = (double)(m->len) / (double)m->nb_buckets;
    int8_t i8 = 0;
    int16_t i16 = 0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    va_list args;
    key_s k;

    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        rehash(m);
    }

    k = make_key(m, key, key_len);

    if (m->value_len == 0) {
        /* Special case for empty values, used to implement sets. */
        insert(m, &k, NULL);
        return;
    }

    va_start(args, key_len);
    i64 = va_arg(args, int64_t);
    va_end(args);

    switch (m->value_len) {
        case sizeof(int8_t):
            i8 = (int8_t)i64;
            insert(m, &k, &i8);
            return;
        case sizeof(int16_t):
            i16 = (int16_t)i64;
            insert(m, &k, &i16);
            return;
        case sizeof(int32_t):
            i32 = (int32_t)i64;
            insert(m, &k, &i32);
            return;
        case sizeof(int64_t):
            insert(m, &k, &i64);
            return;
        default:
            assert(0 && "unsupported value data size");
    }
}

void* map_at(const map_t map, const void* key, size_t key_len) {
    const map_s* m = map;
    const key_s k = make_key(m, key, key_len);
    const elem_s elem = find_key(m, &k);
    return elem.found ? bucket_val(m, elem.bucket, elem.pos) : NULL;
}

int map_del(map_t map, const void* key, size_t key_len) {
    map_s* m = map;
    const key_s k = make_key(m, key, key_len);
    const elem_s elem = find_key(m, &k);

    if (!elem.found) {
        return 0;
    }

    if (elem.bucket->len > 1) {
        const size_t last = elem.bucket->len - 1;
        elem.bucket->hashes[elem.pos] = elem.bucket->hashes[last];
        elem.bucket->keys[elem.pos] = elem.bucket->keys[last];
        memcpy(bucket_val(m, elem.bucket, elem.pos),
               bucket_val(m, elem.bucket, last), m->value_len);
    }
    --elem.bucket->len;
    --m->len;
    return 1;
}

int map_iter(const map_t map, map_it_t* it) {
    const map_s* m = map;

    if (m->len == 0) {
        return 0;
    }

    if (it->_b == NULL) {
        it->_b = &m->buckets[0];
    }

    while (it->_bpos < m->nb_buckets) {
        bucket_s* b = it->_b;
        for (; b != NULL; it->_b = b = b->next, it->_kpos = 0) {
            if (it->_kpos >= b->len) {
                continue;
            }
            it->key = m->keys.data + b->keys[it->_kpos];
            it->key_len = (m->key_size == KEY_SIZE_MIXED ||
                           m->key_size == KEY_SIZE_UNKNOWN)
                              ? (size_t)strlen(it->key)
                              : (size_t)m->key_size;
            it->value = bucket_val(m, b, it->_kpos);
            ++it->_kpos;
            return 1;
        }
        it->_kpos = 0;
        if (++it->_bpos == m->nb_buckets) {
            return 0;
        }
        it->_b = &m->buckets[it->_bpos];
    }
    return 0;
}
