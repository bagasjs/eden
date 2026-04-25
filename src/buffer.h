/// TODO:
/// Currently buffer is just an API that has new, free, insert, delete, getitem, setitem, length
/// It can be implemented as dynamic array of characters, rope data structure, gap buffer, etc.
/// The point is we need another struct that wrap this buffer data structure that contains 
/// the editing state of that buffer (state could be cursor, lines, etc)
///
/// Editor like [Lite](github.com/lite) called it Doc
#ifndef BUFFER_H_
#define BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef MEMDEBUG
#define MALLOC_WITH_LABEL(SIZE, LABEL) malloc((SIZE))
#define FREE_WITH_LABEL(PTR, LABEL) free((PTR))
#else
#define MALLOC_WITH_LABEL(SIZE, LABEL) __malloc_with_label((SIZE), (LABEL))
#define FREE_WITH_LABEL(PTR, LABEL) __free_with_label((PTR), (LABEL))
void *__malloc_with_label(size_t size, const char *label);
void  __free_with_label(void *ptr, const char *label);
#endif

typedef uint32_t rune;

typedef struct Line Line;
struct Line {
    Line *prev;
    Line *next;

    size_t start;
    size_t end;
};

/**
 * This part is only the Base Buffer structure. You could call it 
 * the base class in OOP term. Note that we might allocate more 
 * than sizeof(Buffer) i.e. for Buffer that uses rope data structure 
 * for the string manipulation will do something like this
 * ```c
 * struct RopeBuffer {
 *     Buffer base;
 *     Node *root;
 * };
 * ```
 */
typedef struct Buffer {
    size_t cursor;
    Line  *current_line;
    struct {
        Line *begin;
        Line *end;
    } lines;
} Buffer;

//////////////////////////////////////
///
/// Implementation Dependent APIs
///

Buffer *buffer_new(void);
void   buffer_destroy(Buffer *buf);
rune   buffer_getitem(Buffer *buffer, size_t index);
void   buffer_setitem(Buffer *buffer, size_t index, rune value);
size_t buffer_length(Buffer *buffer);
void   buffer_dump(Buffer *buffer);

/**
 * This is unsafe because it will make buffer's base state to be invalid
 */
void buffer_unsafe_insert(Buffer *buffer, size_t index, const char *text, size_t text_length);

/**
 * This is unsafe because it will make buffer's base state to be invalid
 */
void buffer_unsafe_delete(Buffer *buffer, size_t start, size_t length);

//////////////////////////////////////
///
/// Implementation Independent APIs
///

/**
 * This is unsafe because it will reset the buffer's base state
 */
void buffer_unsafe_init_base(Buffer *buffer);

void buffer_insert_char(Buffer *buf, rune ch);
void buffer_backspace(Buffer *buf);

void buffer_move_to_char_left(Buffer *buf);
void buffer_move_to_char_right(Buffer *buf);
void buffer_move_to_line_up(Buffer *buf);
void buffer_move_to_line_down(Buffer *buf);
void buffer_move_to_start_of_line(Buffer *buf);
void buffer_move_to_end_of_line(Buffer *buf);
void buffer_move_to_first_line(Buffer *buf);
void buffer_move_to_last_line(Buffer *buf);

#endif // BUFFER_H_ 
