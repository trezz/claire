#ifndef CLAIRE_ITER_H
#define CLAIRE_ITER_H

#include <stddef.h>

typedef void IterYielder2;

typedef void (*IterSeq2Func)(void* ctx, IterYielder2* yielder);

typedef struct {
    void* ctx;
    IterSeq2Func func;
} IterSeq2;

IterSeq2 iterSeq2Create(void* ctx, IterSeq2Func func);

int iterYield2(IterYielder2* yield, void* key, size_t keyLen, void* value,
               size_t valueLen);

typedef int (*IterRange2Func)(void* ctx, void* key, size_t keyLen, void* value,
                              size_t valueLen);

void iterRange2(IterSeq2 seq, void* ctx, IterRange2Func func);

#endif
