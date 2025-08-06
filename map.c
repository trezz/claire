#include "map.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "hash.h"
#include "iter.h"

#define BUCKET_CAP 8
#define BUFFER_CAP 1024
#define MAX_LOAD_FACTOR 6.5

typedef struct {
    size_t hash;
    BufferDescriptor key, value;
} Item;

typedef struct Bucket {
    Item items[BUCKET_CAP];
    size_t len;
    struct Bucket* next;
} Bucket;

typedef struct {
    int seed;

    size_t len, cap;

    size_t bucketsLen;
    Bucket* buckets;

    Buffer buf;
} HashMap;

static void init(HashMap* m, size_t cap) {
    m->seed = rand();

    m->len = 0;
    m->cap = cap;

    assert(cap % BUCKET_CAP == 0 && "Capacity must be multiple of BUCKET_CAP");
    m->bucketsLen = cap;
    m->buckets = malloc(m->bucketsLen * sizeof(Bucket));
    assert(m->buckets != NULL && "Failed to allocate memory for buckets");
    memset(m->buckets, 0, m->bucketsLen * sizeof(Bucket));

    m->buf = bufferCreate(BUFFER_CAP);
}

static void deinit(HashMap* m) {
    size_t i = 0;
    for (i = 0; i < m->bucketsLen; ++i) {
        Bucket* b = m->buckets[i].next;
        while (b != NULL) {
            Bucket* next = b->next;
            free(b);
            b = next;
        }
    }
    free(m->buckets);
    bufferDestroy(m->buf);
}

typedef struct {
    Item item;
    struct Bucket* b;
    size_t bpos;
    int found;
} ItemFound;

/*
 * Find the item in the map by key and return its position.
 * An item is always returned because it represents the position within the map
 * where the key should be or is already present. The `present` field
 * indicates whether the key is found in the map.
 */
static ItemFound findItemFromKey(const HashMap* m, BufferView key) {
    const size_t hash = hashBytes(key.data, key.len, m->seed);
    const size_t bpos = hash & (m->bucketsLen - 1);
    Bucket* b = &m->buckets[bpos];
    size_t i = 0;
    ItemFound res = {0};

    while (1) {
        for (i = 0; i < b->len; ++i) {
            Item* item = &b->items[i];
            if (hash != item->hash) {
                continue;
            }
            if (!strncmp(key.data, bufferDataFromDescriptor(m->buf, item->key),
                         key.len)) {
                res.item.key = item->key;
                res.item.value = item->value;
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

    res.item.hash = hash;
    res.b = b;
    res.bpos = i;
    res.found = i < b->len;
    return res;
}

Map* mapCreate(size_t capacity) {
    HashMap* m = malloc(sizeof(HashMap));
    assert(m != NULL && "Failed to allocate memory for Map");
    if (capacity < BUCKET_CAP) {
        capacity = BUCKET_CAP;
    }
    while (capacity % BUCKET_CAP != 0) {
        capacity++;
    }

    init(m, capacity);
    return m;
}

void mapDestroy(Map* map) {
    deinit(map);
    free(map);
}

size_t mapLen(const Map* map) {
    const HashMap* m = map;
    return m->len;
}

int mapGet(const Map* map, const void* key, size_t keyLen, void* dest,
           size_t* destLen) {
    size_t len = 0;
    void* res = mapAt(map, key, keyLen, &len);
    if (res == NULL) {
        return 0;
    }
    if (destLen != NULL) {
        *destLen = len;
    }
    if (dest != NULL) {
        memcpy(dest, res, len);
    }
    return 1;
}

void* mapAt(const Map* map, const void* key, size_t keyLen, size_t* destLen) {
    const HashMap* m = map;
    BufferView k;
    ItemFound found;
    BufferDescriptor v;

    k.data = (char*)key;
    k.len = keyLen;
    found = findItemFromKey(m, k);
    v = found.item.value;

    if (!found.found) {
        return NULL;
    }
    if (destLen != NULL) {
        *destLen = v.len;
    }
    return bufferDataFromDescriptor(m->buf, v);
}

int mapDelete(Map* map, const void* key, size_t keyLen) {
    HashMap* m = map;
    BufferView k;
    ItemFound found;

    k.data = (char*)key;
    k.len = keyLen;

    found = findItemFromKey(m, k);

    if (!found.found) {
        return 0;
    }

    if (found.b->len > 1) {
        found.b->items[found.bpos] = found.b->items[found.b->len - 1];
    }
    --found.b->len;
    --m->len;
    return 1;
}

static void itemSetValue(HashMap* m, Item* item, BufferView v) {
    if (v.len == 0) {
        item->value.len = 0;
        return;
    }

    if (v.len > item->value.len) {
        item->value = bufferAppendView(&m->buf, v);
        return;
    }

    item->value.len = v.len;
    memcpy(bufferDataFromDescriptor(m->buf, item->value), v.data, v.len);
}

static void insert(HashMap* m, BufferView k, BufferView v) {
    ItemFound found = findItemFromKey(m, k);
    Bucket* b = found.b;
    Item* dest = NULL;

    if (found.found) {
        dest = &b->items[found.bpos];
        itemSetValue(m, dest, v);
        return;
    }

    if (b->len == BUCKET_CAP) {
        b->next = malloc(sizeof(Bucket));
        assert(b->next != NULL && "Failed to allocate memory for new bucket");
        memset(b->next, 0, sizeof(Bucket));
        b = b->next;
        found.bpos = 0;
    }

    dest = &b->items[found.bpos];
    dest->hash = found.item.hash;

    itemSetValue(m, dest, v);
    dest->key = bufferAppendView(&m->buf, k);

    ++b->len;
    ++m->len;
}

static void rehash(HashMap* m) {
    HashMap tmp;
    MapIterator it = mapIteratorCreate(m);

    init(&tmp, m->cap * 2);
    while (mapIteratorNext(&it)) {
        const Item* item = it.item;
        BufferView key = bufferViewFromDescriptor(m->buf, item->key);
        BufferView value = bufferViewFromDescriptor(m->buf, item->value);
        insert(&tmp, key, value);
    }
    deinit(m);
    *m = tmp;
}

void mapSet(Map* map, const void* key, size_t keyLen, const void* value,
            size_t valueLen) {
    HashMap* m = map;
    const double loadFactor = (double)(m->len) / (double)m->bucketsLen;
    BufferView k;
    BufferView v;

    k.data = (char*)key;
    k.len = keyLen;
    v.data = (char*)value;
    v.len = valueLen;

    if (loadFactor > MAX_LOAD_FACTOR) {
        rehash(m);
    }

    insert(m, k, v);
}

MapIterator mapIteratorCreate(const Map* map) {
    const HashMap* m = map;
    MapIterator it;
    it.map = m;
    it.bucket = &m->buckets[0];
    it.bucketPos = 0;
    it.keyPos = 0;
    return it;
}

int mapIteratorNext(MapIterator* it) {
    const HashMap* m = it->map;
    size_t bucketsCount = m->bucketsLen;

    if (m->len == 0) {
        return 0;
    }

    while (it->bucketPos < bucketsCount) {
        const Bucket* b = it->bucket;
        for (; b != NULL; it->bucket = b = b->next, it->keyPos = 0) {
            if (it->keyPos >= b->len) {
                continue;
            }
            it->item = &b->items[it->keyPos];
            ++it->keyPos;
            return 1;
        }
        it->keyPos = 0;
        if (++it->bucketPos == bucketsCount) {
            return 0;
        }
        it->bucket = &m->buckets[it->bucketPos];
    }
    return 0;
}

void* mapIteratorKey(const MapIterator* it, size_t* keyLen) {
    const Item* i = it->item;
    const HashMap* m = it->map;
    if (keyLen != NULL) {
        *keyLen = i->key.len;
    }
    return bufferDataFromDescriptor(m->buf, i->key);
}

void* mapIteratorValue(const MapIterator* it, size_t* valueLen) {
    const Item* i = it->item;
    const HashMap* m = it->map;

    if (valueLen != NULL) {
        *valueLen = i->value.len;
    }
    return bufferDataFromDescriptor(m->buf, i->value);
}

static void mapYieldAll(void* ctx, IterYielder2* yielder) {
    MapIterator it = mapIteratorCreate(ctx);
    size_t keyLen = 0;
    size_t valueLen = 0;
    void* key = NULL;
    void* value = NULL;

    while (mapIteratorNext(&it)) {
        key = mapIteratorKey(&it, &keyLen);
        value = mapIteratorValue(&it, &valueLen);
        if (iterYield2(yielder, key, keyLen, value, valueLen) == 0) {
            return;
        }
    }
}

IterSeq2 mapAll(const Map* map) {
    return iterSeq2Create((void*)map, mapYieldAll);
}
