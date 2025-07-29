#ifndef CLAIRE_MAP_H
#define CLAIRE_MAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * A generic in-memory key-value store.
 */
typedef void *map_t;

/*
 * Returns a new map configured for the the given value size and capacity.
 *
 * Value size can be 0 if the map is used as a set (i.e. no values are stored).
 *
 * Capacity is the number of key-value pairs that can be stored in the map
 * before it needs to be rehashed. If the capacity is 0, a default capacity
 * is used.
 */
map_t map_new(size_t value_len, size_t capacity);

/*
 * Frees the given map.
 */
void map_free(map_t map);

/*
 * Returns the number of key-value pairs in the map.
 */
size_t map_len(map_t map);

/*
 * Retrieves the value associated with the given key.
 * The value is copied to the address pointed to by dest.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int map_get(map_t map, const void *key, size_t key_len, void *dest);

/*
 * Adds or updates a key-value pair in the map.
 *
 * Keys can be of any type. Once the first key is added to the map, any
 * subsequent addition with a different key type is considered undefined
 * behavior.
 * Keys with variable length (e.g. strings) must not contain null bytes.
 *
 * Values can be integers, pointers or they can be ommitted if the map is
 * configured to not hold values (e.g. created with a value size of 0).
 *
 * The map doesn't take ownership of pointer values, so they must remain valid
 * until the key is removed from the map. Keys however are copied in the map.
 * If the key already exists in the map, its value is replaced with the new
 * value.
 */
void map_set(map_t map, const void *key, size_t key_len, ...);

/*
 * Returns a pointer to the value associated with the given key if it exists,
 * or NULL if the key is not found.
 */
void *map_at(map_t map, const void *key, size_t key_len);

/*
 * Deletes the key-value pair associated with the given key.
 * Returns 0 if the key was not found, a non-zero value otherwise.
 */
int map_del(map_t map, const void *key, size_t key_len);

/*
 * An iterator on a map.
 */
typedef struct {
    /* Current key. */
    const void *key;
    /* Length of the current key. */
    size_t key_len;
    /* Pointer on the current value. */
    void *value;

    /* Internal state. */
    void *_b;
    size_t _bpos;
    size_t _kpos;
} map_it_t;

/*
 * Iterate over the map by moving the iterator to the next key/value pair of the
 * map, or to the first key/value pair if the iterator was just initialized.
 * Returns 0 if the iteration reached the end of the map, a non-zero value
 * otherwise.
 *
 * The given iterator must be initialized to zero before the first call, like:
 *      map_it_t it = {0};
 */
int map_iter(const map_t map, map_it_t *it);

#endif
