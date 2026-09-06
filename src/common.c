#include "common.h"
#include <stdlib.h>
#include <stdio.h>

void *__malloc_with_label(size_t size, const char *label)
{
    void *ptr = malloc(size);
    printf("[TRACE] MALLOC(%p): %zu bytes: %s\n", ptr, size, label);
    return ptr;
}

void  __free_with_label(void *ptr, const char *label)
{
    printf("[TRACE] FREE(%p): %s\n", ptr, label);
    free(ptr);
}

bool sv_eq(StringView a, StringView b)
{
    if(a.count != b.count) return false;
    for(size_t i = 0; i < a.count; ++i) {
        if(a.items[i] != b.items[i]) return false;
    }
    return true;
}
