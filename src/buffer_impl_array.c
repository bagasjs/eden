#include "buffer.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct ArrayBuffer {
    Buffer base;

    char  *items;
    size_t count;
    size_t capacity;
} ArrayBuffer;

#define INIT_CAP 256
Buffer *buffer_unsafe_new(void)
{
    ArrayBuffer *buf = MALLOC_WITH_LABEL(sizeof(*buf), "ArrayBuffer");
    buf->capacity = INIT_CAP;
    buf->items    = MALLOC_WITH_LABEL(buf->capacity, "ArrayBuffer.items");
    buf->count    = 0;

    return (Buffer*)buf;
}

void buffer_unsafe_destroy(Buffer *base)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;
    FREE_WITH_LABEL(buf->items, "ArrayBuffer.items");
    FREE_WITH_LABEL(buf, "ArrayBuffer");
}

void buffer_unsafe_insert(Buffer *base, size_t index, const char *text, size_t text_length)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;

    size_t reqsz = buf->count + text_length + 1;
    if(reqsz > buf->capacity) {
        while(buf->capacity < reqsz) buf->capacity *= 2;
        char *new = MALLOC_WITH_LABEL(buf->capacity, "Resizing ArrayBuffer.items");
        assert(new && "Buy more RAM LOL!");
        memcpy(new, buf->items, buf->count);
        FREE_WITH_LABEL(buf->items, "Removing old ArrayBuffer.items");
        buf->items = new;
    }

    if(index < buf->count) {
        // ABC DEFG
        // 012 3456
        //     ^ Move to here
        // ABC XYZ DEFG
        // 012 345 6789
        memmove(buf->items + index + text_length, buf->items + index, buf->count - index);
        memmove(buf->items + index, text, text_length);
    } else {
        memmove(buf->items + buf->count, text, text_length);
    }
    buf->count += text_length;
}

void buffer_unsafe_delete(Buffer *base, size_t start, size_t length)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;

    if(start > buf->count) return;
    // ABCDEFGHIJ
    // 0123456789
    //        ^--
    size_t right = buf->count - start;
    if(right < length) length = right;
    // ABCDEFGHIJ
    // 0123456789
    //      ^--
    memmove(buf->items + start, buf->items + start + length, right - length);
    buf->count -= length;
}

rune buffer_getitem(Buffer *base, size_t index)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;

    if(index > buf->count) return 0;
    return buf->items[index];
}

void buffer_setitem(Buffer *base, size_t index, rune value)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;
    if(index > buf->count) return;
    buf->items[index] = value;
}

size_t buffer_length(Buffer *base)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;
    return buf->count;
}

void buffer_dump(Buffer *base)
{
    ArrayBuffer *buf = (ArrayBuffer*)base;
    printf("BUFFER: length=%zu\n", buf->count);
    printf("%.*s\n", (int)buf->count, buf->items);
}
