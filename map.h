#ifndef CLAIRE_MAP_H
#define CLAIRE_MAP_H

#include <stddef.h>
#include <stdint.h>

#include "buffer.h"

/*
 * A generic in-memory key-value store.
 */
typedef void Map;

/*
 * Creates a new map configured with the given capacity, which is the number of
 * key-value pairs that can be stored in the map before it needs to be rehashed.
 * If the capacity is 0, a default capacity is used.
 */
Map *mapCreate(size_t capacity);

/*
 * Destroys the given map.
 */
void mapDestroy(Map *map);

/*
 * Returns the number of key-value pairs in the map.
 */
size_t mapLen(const Map *map);

/*
 * Retrieves the value associated with the given key.
 * The value is copied to the address pointed to by dest and its length is
 * stored in destLen if destLen is not NULL.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int mapGet(const Map *map, const void *key, size_t keyLen, void *dest,
           size_t *destLen);

/*
 * Retrieves the value associated with the given key.
 * The value is not copied, but a pointer to the data in the map is returned.
 * If destLen is not NULL, it is set to the length of the value.
 * Returns NULL if the key was not found, a pointer to the value otherwise.
 */
void *mapAt(const Map *map, const void *key, size_t keyLen, size_t *destLen);

/*
 * Deletes the key-value pair associated with the given key.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int mapDelete(Map *map, const void *key, size_t keyLen);

/*
 * Adds or updates a key-value pair in the map.
 *
 * Values can be NULL if the map is used as a set.
 *
 * Keys and values are copied in the map. If the key already exists in the
 * map, its value is replaced with the new value.
 */
void mapSet(Map *map, const void *key, size_t keyLen, const void *value,
            size_t valueLen);

/*
 * An iterator for traversing the map. Its fields are internal state and should
 * not be modified directly.
 * It is valid to pass an uninitialized iterator to mapNextKey, which will
 * initialize it.
 */
typedef struct {
    const Map *map;
    const void *bucket;
    const void *item;
    size_t bucketPos;
    size_t keyPos;
} MapIterator;

/*
 * Iterator over the keys of the map.
 */
int mapNextKey(const Map *map, MapIterator *it, void *key, size_t *keyLen);

#endif
