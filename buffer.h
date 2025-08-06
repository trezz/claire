#ifndef CLAIRE_BUFFER_H
#define CLAIRE_BUFFER_H

#include <stddef.h>

/*
 * A dynamic buffer that can grow as needed.
 * It is used to store data in a contiguous memory area.
 */
typedef struct {
    size_t cap;
    size_t len;
    char* data;
} Buffer;

/*
 * A view of a part of a buffer.
 * The view has direct access to the data in the buffer but does not own it.
 * If the underlying buffer changes (reallocated or destroyed), the view may
 * become invalid.
 */
typedef struct {
    char* data;
    size_t len;
} BufferView;

/*
 * A descriptor of a part of a buffer.
 * Unlike BufferView, it does not have direct access to the data in the buffer.
 * It can be used to keep access to buffered data even if the buffer is
 * reallocated.
 */
typedef struct {
    size_t off;
    size_t len;
} BufferDescriptor;

/*
 * Creates a new buffer with the given capacity.
 * The buffer is initially empty.
 */
Buffer bufferCreate(size_t cap);

/*
 * Destroys the given buffer and frees its memory.
 */
void bufferDestroy(Buffer b);

/*
 * Appends a view to the buffer and returns a descriptor of the appended data.
 * If the buffer does not have enough capacity, it is reallocated to fit the new
 * data.
 */
BufferDescriptor bufferAppendView(Buffer* b, BufferView v);

/*
 * Creates a view of a part of the buffer at the given offset and length.
 */
BufferView bufferView(Buffer b, size_t off, size_t len);

/*
 * Creates a view of a part of the buffer from a descriptor.
 * The descriptor must be valid for the given buffer.
 */
BufferView bufferViewFromDescriptor(Buffer b, BufferDescriptor d);

/*
 * Returns a pointer to the data in the buffer from a descriptor.
 * The descriptor must be valid for the given buffer.
 */
char* bufferDataFromDescriptor(Buffer b, BufferDescriptor d);

#endif
