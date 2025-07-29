#ifndef MAP_H
#define MAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * Map is a generic in-memory key-value store.
 *
 * A map can be used as a set by setting the value size to 0 and omitting
 * the value when adding a key.
 */
typedef void *map_t;

/*
 * Returns a new map configured for the the given value size and capacity.
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
 * Retrieves the value associated with the given key.
 * A pointer to the value is returned if the key was found.
 * NULL is returned if the key was not found.
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
 * Iterate over the map.
 * The provided iterator must be initialized to zero before the first call.
 * The function moves the iterator to the next key/value pair of the map, or to
 * the first key/value pair if the iterator was just initialized.
 * If the function returns 0, the iteration reached the end of the map and the
 * iterator state is undefined, otherwise the iterator is pointing to a
 * key/value pair of the map.
 */
int map_iter(const map_t map, map_it_t *it);

#endif
