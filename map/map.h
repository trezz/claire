#ifndef MAP_H
#define MAP_H

#include <stddef.h>

/*
 * Map is a generic in-memory key-value store.
 */
typedef void *map_t;

/*
 * Returns a new map with the given value size and capacity.
 * NULL is returned in case of error.
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
 * Adds the given key and the associated value passed by value in the map.
 * Use this function to map strings to literal values like integers or pointers.
 */
void map_add(map_t map, const void *key, size_t key_len, ...);

/*
 * An iterator on a map.
 */
typedef struct strmap_iterator {
    /* Current key. */
    const void *key;
    size_t key_len;
    /* Pointer on the current value. */
    void *val_ptr;

    /* Internal state. */
    map_t _map;
    void *_b;
    size_t _bpos;
    size_t _kpos;
} map_iterator_t;

/*
 * Returns an iterator on the map.
 * The returned iterator is initialized to iterate on the map, but doesn't
 * points to any key/value pair yet. A call to map_iterator_next is required to
 * set the iterator on the first key/value pair.
 */
map_iterator_t map_iterator(const map_t map);

/*
 * Move the given iterator to the next key/value pair of the map, or to the
 * first key/value pair if the iterator was just initialized by map_iterator.
 * If the function returns 0, the iteration reached the end of the map and the
 * iterator state is undefined, otherwise the iterator is pointing to a
 * key/value pair of the map.
 */
int map_iterator_next(map_iterator_t *it);

#endif
