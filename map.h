#ifndef CLAIRE_MAP_H
#define CLAIRE_MAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * A generic in-memory key-value store.
 */
typedef void Map;

/*
 * Returns a new map configured for the the given value size and capacity.
 *
 * Value size can be 0 if the map is used as a set (i.e. no values are stored).
 *
 * Capacity is the number of key-value pairs that can be stored in the map
 * before it needs to be rehashed. If the capacity is 0, a default capacity
 * is used.
 */
Map *mapNew(size_t valueLen, size_t capacity);

/*
 * Frees the given map.
 */
void mapFree(Map *m);

/*
 * Returns the number of key-value pairs in the map.
 */
size_t mapLen(const Map *m);

/*
 * Retrieves the value associated with the given key.
 * The value is copied to the address pointed to by dest.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int mapGet(const Map *m, const void *key, size_t keyLen, void *dest);

/*
 * Adds or updates a key-value pair in the map.
 *
 * Keys can be of any type. Once the first key is added to the map, any
 * subsequent addition with a different key type is considered undefined
 * behavior.
 * Keys with variable length (e.g. strings) must not contain null bytes.
 *
 * Values can be NULL if the map is configured to not hold values (e.g. created
 * with a value size of 0).
 *
 * Keys are copied in the map. If the key already exists in the map, its value
 * is replaced with the new value.
 */
void mapSet(Map *m, const void *key, size_t keyLen, const void *value);

/*
 * Returns a pointer to the value associated with the given key if it exists,
 * or NULL if the key is not found.
 */
void *mapAt(const Map *m, const void *key, size_t keyLen);

/*
 * Deletes the key-value pair associated with the given key.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int mapDelete(Map *m, const void *key, size_t keyLen);

/*
 * An iterator on a map.
 */
typedef struct {
    /* Current key. */
    const void *key;
    /* Length of the current key. */
    size_t keyLen;
    /* Pointer on the current value. */
    void *value;

    /* Internal state. */
    void *_b;
    size_t _bpos;
    size_t _kpos;
} MapIt;

/*
 * Iterate over the map by moving the iterator to the next key/value pair of the
 * map, or to the first key/value pair if the iterator was just initialized.
 * Returns 0 if the iteration reached the end of the map, a non-zero value
 * otherwise.
 *
 * The given iterator must be initialized to zero before the first call, like:
 *      MapIt it = {0};
 */
int mapIter(const Map *m, MapIt *it);

#endif
