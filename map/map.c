#include "map.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../hash/hash.h"

#define HASH_SEED 13
#define BUCKET_CAPA 8
#define KEY_CAPA 1024
#define MAP_MAX_LOAD_FACTOR 6.5
#define KEY_SIZE_MIXED SIZE_MAX
#define KEY_SIZE_UNKNOWN 0

typedef struct bucket {
    size_t hash[BUCKET_CAPA];
    size_t key_positions[BUCKET_CAPA];
    char* values;
    size_t len;
    struct bucket* next;
} bucket_s;

static void init_bucket(bucket_s* b, size_t value_len) {
    b->values = malloc(value_len * BUCKET_CAPA);
    assert(b->values != NULL && "Failed to allocate memory for bucket values");
    b->len = 0;
    b->next = NULL;
}

static void deinit_bucket(bucket_s* b) {
    free(b->values);
    if (b->next != NULL) {
        deinit_bucket(b->next);
    }
}

typedef struct map {
    size_t hash_seed;
    size_t key_size;
    size_t value_len;
    size_t capacity;
    size_t len;

    size_t nb_buckets;
    bucket_s* buckets;

    char* keys;
    size_t keys_len;
    size_t keys_capacity;
} map_s;

static void init_map(map_s* m, size_t value_len, size_t capacity) {
    size_t i = 0;

    m->hash_seed = HASH_SEED;
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

    m->keys = malloc(KEY_CAPA);
    assert(m->keys != NULL && "Failed to allocate memory for map keys");
    m->keys_len = 0;
    m->keys_capacity = KEY_CAPA;

    for (i = 0; i < m->nb_buckets; ++i) {
        init_bucket(&m->buckets[i], value_len);
    }
}

static void deinit_map(map_s* m) {
    size_t i = 0;
    for (i = 0; i < m->nb_buckets; ++i) {
        deinit_bucket(&m->buckets[i]);
    }
    free(m->buckets);
    free(m->keys);
}

map_t map_new(size_t value_len, size_t capacity) {
    map_s* m = NULL;
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

/*
 * Finds the map bucket holding the given key and returns 1 if the key was
 * found. If the key is not in the map, 0 is returned and the bucket that is
 * expected to store the key is set in found_bucket.
 */
static int find_bucket_pos(const map_s* m, const char* key, size_t key_len,
                           unsigned long* out_key_hash, bucket_s** found_bucket,
                           size_t* found_pos) {
    const unsigned long h = hash_bytes(key, key_len, m->hash_seed);
    const size_t bpos = bucket_pos(m, h);
    bucket_s* b = &m->buckets[bpos];
    size_t i = 0;

    *out_key_hash = h;

    while (1) {
        for (i = 0; i < b->len; ++i) {
            const char* bkey = NULL;
            if (h != b->hash[i]) {
                continue;
            }
            bkey = m->keys + b->key_positions[i];
            if (!strncmp(key, bkey, key_len)) {
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

    *found_bucket = b;
    *found_pos = i;

    return i < b->len;
}

int map_get(const map_t map, const void* key, size_t key_len, void* v) {
    const map_s* m = map;
    const void* data = map_at(map, key, key_len);
    if (data == NULL) {
        return 0;
    }
    if (v != NULL) {
        memcpy(v, data, m->value_len);
    }
    return 1;
}

/*
 * Appends the given key in the keys buffer of the map and returns its position.
 */
static size_t append_new_key(map_s* m, const char* key, size_t key_len) {
    size_t key_pos = 0;

    while (m->keys_len + key_len + 1 > m->keys_capacity) {
        m->keys_capacity *= 2;
        m->keys = realloc(m->keys, m->keys_capacity);
        assert(m->keys != NULL && "Failed to reallocate memory for map keys");
    }

    key_pos = m->keys_len;
    memcpy(m->keys + key_pos, key, key_len);
    m->keys[key_pos + key_len] = '\0';
    m->keys_len += key_len + 1;

    return key_pos;
}

/*
 * Insert a new key/value pair in the map and return a pointer to the map.
 */
static void insert(map_s* m, const char* key, size_t key_len,
                   const void* val_ptr) {
    bucket_s* b = NULL;
    size_t pos = 0;
    size_t h = 0;

    if (find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        if (val_ptr != NULL) {
            memcpy(bucket_val(m, b, pos), val_ptr, m->value_len);
        }
        return;
    }
    pos = b->len;

    if (pos == BUCKET_CAPA) {
        b->next = malloc(sizeof(bucket_s));
        assert(b->next != NULL && "Failed to allocate memory for new bucket");
        init_bucket(b->next, m->value_len);
        b = b->next;
        pos = 0;
    }
    if (val_ptr != NULL) {
        memcpy(bucket_val(m, b, pos), val_ptr, m->value_len);
    }
    b->hash[pos] = h;
    b->key_positions[pos] = append_new_key(m, key, key_len);

    if (m->key_size == KEY_SIZE_UNKNOWN) {
        m->key_size = key_len;
    } else if (m->key_size != key_len) {
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
        insert(&tmp, it.key, it.key_len, it.value);
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

    if (load_factor > MAP_MAX_LOAD_FACTOR) {
        rehash(m);
    }

    if (m->value_len == 0) {
        /* Special case for empty values, used to implement sets. */
        insert(m, key, key_len, NULL);
        return;
    }

    va_start(args, key_len);
    i64 = va_arg(args, int64_t);
    va_end(args);

    switch (m->value_len) {
        case sizeof(int8_t):
            i8 = (int8_t)i64;
            insert(m, key, key_len, &i8);
            return;
        case sizeof(int16_t):
            i16 = (int16_t)i64;
            insert(m, key, key_len, &i16);
            return;
        case sizeof(int32_t):
            i32 = (int32_t)i64;
            insert(m, key, key_len, &i32);
            return;
        case sizeof(int64_t):
            insert(m, key, key_len, &i64);
            return;
        default:
            assert(0 && "unsupported value data size");
    }
}

void* map_at(const map_t map, const void* key, size_t key_len) {
    const map_s* m = map;
    bucket_s* b = NULL;
    size_t pos = 0;
    size_t h = 0;

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return NULL;
    }
    return bucket_val(m, b, pos);
}

int map_del(map_t map, const void* key, size_t key_len) {
    map_s* m = map;
    bucket_s* b = NULL;
    size_t pos = 0;
    size_t h = 0;

    if (!find_bucket_pos(m, key, key_len, &h, &b, &pos)) {
        return 0;
    }
    if (b->len > 1) {
        b->hash[pos] = b->hash[b->len - 1];
        b->key_positions[pos] = b->key_positions[b->len - 1];
        memcpy(bucket_val(m, b, pos), bucket_val(m, b, b->len - 1),
               m->value_len);
    }
    --b->len;
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
            it->key = m->keys + b->key_positions[it->_kpos];
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
