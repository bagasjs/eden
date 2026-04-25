#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>

void *__malloc_with_label(size_t size, const char *label)
{
    printf("[TRACE] MALLOC: %s\n", label);
    return malloc(size);
}

void  __free_with_label(void *ptr, const char *label)
{
    printf("[TRACE] FREE: %s\n", label);
    free(ptr);
}

void buffer_unsafe_init_base(Buffer *buffer)
{
    buffer->cursor = 0;
    buffer->lines.begin = NULL;
    buffer->lines.end   = NULL;
    buffer->current_line = NULL;
}

void buffer_insert_char(Buffer *buf, rune ch)
{
    buffer_unsafe_insert(buf, buf->cursor, (char*)&ch, 1);
    buf->cursor += 1;
}

void buffer_backspace(Buffer *buf)
{
    if(buf->cursor == 0) return;
    buffer_unsafe_delete(buf, buf->cursor - 1, 1);
    buf->cursor -= 1;
}

void buffer_move_to_char_left(Buffer *buf)
{
    // TODO: we want move_to_char_left not working if it's the
    //       end of current line (just like VIM)
    if(buf->cursor != 0) buf->cursor -= 1;
}

void buffer_move_to_char_right(Buffer *buf)
{
    // TODO: we want move_to_char_right not working if it's the
    //       start of current line (just like VIM)
    if(buf->cursor < buffer_length(buf)) buf->cursor += 1;
}


