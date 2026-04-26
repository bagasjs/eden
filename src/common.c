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


