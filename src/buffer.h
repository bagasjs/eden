/// TODO:
///
/// Okay, the previous todo is cool, but in the end we kinda mixed the state of cursor editing state
/// and the buffer's state in a same representation which is Buffer. I have find a good name to 
/// separate the buffer's state (consist of filename, lines, text data) and the cursor editing state.
/// This is very useful in the case where in the future we will need to support multiple cursor, multiple
/// panels or tabs editing the same file/buffer. Thus separating this 2 thing is important.
/// There will be Buffer and BufferView. Preferably Buffer and BufferView would look like the following
///
/// ```c
/// struct Buffer {
///     [ Common properties]
///     Lines lines;
///     const char *filepath
///     ... [ Per implementation detail ]
/// }
///
/// struct BufferView {
///     size_t cursor;
///     size_t current_lines;
/// }
/// ```
///
///
#ifndef BUFFER_H_
#define BUFFER_H_

#include "common.h"
#include <stddef.h>
#include <stdint.h>

typedef uint32_t rune;

typedef struct Line Line;
struct Line {
    size_t start;
    size_t end;
};

typedef struct Lines {
    Line *items;
    size_t count;
    size_t capacity;
} Lines;

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
    const char *filepath;
    Lines  lines;

    size_t cursor;
    size_t current_line;
} Buffer;

//////////////////////////////////////
///
/// Implementation Dependent APIs
///

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

/**
 * This is unsafe because it won't initialize the base buffer's state
 */
Buffer *buffer_unsafe_new(void);

/**
 * This is unsafe because it won't deinitialize the base buffer's state
 */
void buffer_unsafe_destroy(Buffer *buf);


//////////////////////////////////////
///
/// Implementation Independent APIs
///

Buffer *buffer_new(void);
void buffer_destroy(Buffer *buffer);

void buffer_insert_char(Buffer *buf, rune ch);
void buffer_backspace(Buffer *buf);

void buffer_move_to_char_left(Buffer *buf);
void buffer_move_to_char_right(Buffer *buf);
void buffer_move_to_line_above(Buffer *buf);
void buffer_move_to_line_below(Buffer *buf);
void buffer_move_to_start_of_line(Buffer *buf);
void buffer_move_to_end_of_line(Buffer *buf);
void buffer_move_to_first_line(Buffer *buf);
void buffer_move_to_last_line(Buffer *buf);
void buffer_move_to_line(Buffer *buf, size_t line_number, size_t line_offset);


void buffer__debug(Buffer *buf);

#endif // BUFFER_H_ 
