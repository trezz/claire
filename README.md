# Claire

A lightweight, no-dependency C89 library providing hash functions and hash maps for systems programming.

## Features

### Hash Module (`hash.h`)
Fast MurmurHash3 implementation optimized for 32-bit and 64-bit systems.

- Cross-platform optimization
- Configurable hash seeds
- High performance

### Map Module (`map.h`) 
Generic hash table supporting arbitrary key-value types. Can also be used as a set.

- Generic design for any data types
- Variable-length keys (strings, binary data)
- Automatic resizing
- Memory efficient
- Iterator support

## Building

```bash
# Build production library
make lib

# Build debug library with symbols
make lib-debug

# Build and run tests
make test

# Build everything (default)
make all
```

## Output

- `build/lib/libclaire.a` - Production static library
- `build/lib/libclaire-debug.a` - Debug static library with symbols

## Usage

```c
#include "hash.h"
#include "map.h"
```

C89 compliant with no external dependencies.