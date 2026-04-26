#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef MEMDEBUG
#define MALLOC_WITH_LABEL(SIZE, LABEL) malloc((SIZE))
#define FREE_WITH_LABEL(PTR, LABEL) free((PTR))
#else
#define MALLOC_WITH_LABEL(SIZE, LABEL) __malloc_with_label((SIZE), (LABEL))
#define FREE_WITH_LABEL(PTR, LABEL) __free_with_label((PTR), (LABEL))
void *__malloc_with_label(size_t size, const char *label);
void  __free_with_label(void *ptr, const char *label);
#endif

#define DA_INIT_CAP 32
#define da_append(da, item) do {                                                     \
        if((da)->count >= (da)->capacity) {                                          \
            if((da)->capacity == 0) (da)->capacity = DA_INIT_CAP;                    \
            while((da)->capacity < (da)->count) (da)->capacity *= 2;                 \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                            \
        (da)->items[(da)->count++] = (item);                                         \
    } while(0)

#define da_insert(da, item, index)                                                   \
    do {                                                                             \
        if((da)->count >= (da)->capacity) {                                          \
            if((da)->capacity == 0) (da)->capacity = DA_INIT_CAP;                    \
            while((da)->capacity < (da)->count) (da)->capacity *= 2;                 \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                            \
        if((index) < (da)->count) {                                                  \
            memmove((da)->items + (index) + 1, (da)->items + (index),                \
                    (da)->count - (index));                                          \
            (da)->items[index] = (item);                                             \
        } else {                                                                     \
            (da)->items[(da)->count] = (item);                                       \
        }                                                                            \
    } while(0)

#define da_remove(da, index)                                        \
    do {                                                            \
        if((index) > (da)->start) break;                            \
        size_t right = (da)->count - (index);                       \
        memmove((da)->items + (index), (da)->items + (index) + 1,   \
                (da)->count - (index) - 1);                         \
    } while(0)


#endif // COMMON_H_
