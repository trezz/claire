#include "buffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Buffer bufferCreate(size_t cap) {
    Buffer b;
    b.cap = cap;
    b.len = 0;
    b.data = malloc(cap);
    assert(b.data != NULL && "Failed to allocate memory for buffer");
    return b;
}

void bufferDestroy(Buffer b) { free(b.data); }

BufferDescriptor bufferAppendView(Buffer* b, BufferView v) {
    BufferDescriptor d;
    if (b->len + v.len > b->cap) {
        b->cap = (b->cap + v.len) * 2;
        b->data = realloc(b->data, b->cap);
        assert(b->data != NULL && "Failed to reallocate memory for buffer");
    }
    memcpy(b->data + b->len, v.data, v.len);
    d.off = b->len;
    d.len = v.len;
    b->len += v.len;
    return d;
}

BufferView bufferView(Buffer b, size_t off, size_t len) {
    BufferView v;
    v.data = b.data + off;
    v.len = len;
    return v;
}

BufferView bufferViewFromDescriptor(Buffer b, BufferDescriptor d) {
    return bufferView(b, d.off, d.len);
}

char* bufferDataFromDescriptor(Buffer b, BufferDescriptor d) {
    BufferView v = bufferViewFromDescriptor(b, d);
    return v.data;
}
