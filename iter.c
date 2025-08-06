#include "iter.h"

IterSeq2 iterSeq2Create(void* ctx, IterSeq2Func func) {
    IterSeq2 seq;
    seq.ctx = ctx;
    seq.func = func;
    return seq;
}

typedef int (*IterYield2Func)(void* ctx, void* k, size_t klen, void* v,
                              size_t vlen);

typedef struct {
    void* ctx;
    IterYield2Func func;
} Yielder2;

int iterYield2(IterYielder2* yield, void* key, size_t keyLen, void* value,
               size_t valueLen) {
    Yielder2* yielder = (Yielder2*)yield;
    return yielder->func(yielder->ctx, key, keyLen, value, valueLen);
}

void iterRange2(IterSeq2 seq, void* ctx, IterRange2Func func) {
    Yielder2 yielder;
    yielder.ctx = ctx;
    yielder.func = func;
    seq.func(seq.ctx, &yielder);
}
