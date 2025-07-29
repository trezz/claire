#ifndef HASH_H
#define HASH_H

#include <stddef.h>

/*
 * Returns the hash of the value of size len pointed to by ptr using the
 * provided seed.
 * The hash is computed using the MurmurHash3 algorithm taken from the GNU ISO
 * C++ Standard Library.
 */
size_t hash_bytes(const void *ptr, size_t len, size_t seed);

#endif
