#include "buffer.h"
#include "common.h"
#include <string.h>
#include <stdio.h>

void buffer__debug(Buffer *buf)
{
    for(size_t i = 0; i < buf->lines.count; ++i) {
        Line line = buf->lines.items[i];
        printf("%zu [%zu - %zu]| ", i, line.start, line.end);
        for(size_t j = line.start; j < line.end; ++j) {
            printf("%c", buffer_getitem(buf, j));
        }
        printf("\n");
    }
}


static void buffer_update_lines(Buffer *buffer)
{
    buffer->lines.count = 0;
    Line curr = {0};
    for(size_t i = 0; i < buffer_length(buffer); ++i) {
        rune ch = buffer_getitem(buffer, i);
        if(buffer->cursor == i + 1) {
            buffer->current_line = buffer->lines.count;
        }
        if(ch == '\n') {
            curr.end = i;
            da_append(&buffer->lines, curr);
            curr.start = i + 1;
            continue;
        }
    }
    curr.end = buffer_length(buffer);
    da_append(&buffer->lines, curr);
}

Buffer *buffer_new(void)
{
    Buffer *buffer = buffer_unsafe_new();
    memset(buffer, 0, sizeof(*buffer));
    buffer->cursor = 0;
    buffer->current_line = 0;
    buffer_update_lines(buffer);
    return buffer;
}

void buffer_destroy(Buffer *buffer)
{
    free(buffer->lines.items);
    buffer_unsafe_destroy(buffer);
}

void buffer_insert_char(Buffer *buf, rune ch)
{
    buffer_unsafe_insert(buf, buf->cursor, (char*)&ch, 1);
    buf->cursor += 1;
    buffer_update_lines(buf);
}

void buffer_backspace(Buffer *buf)
{
    if(buf->cursor == 0) return;
    buffer_unsafe_delete(buf, buf->cursor - 1, 1);
    buf->cursor -= 1;
    buffer_update_lines(buf);
}

void buffer_move_to_line_above(Buffer *buf)
{
    if(buf->current_line != 0) {
        buf->cursor = buf->lines.items[buf->current_line - 1].end;
        buf->current_line -= 1;
    }
}

void buffer_move_to_line_below(Buffer *buf)
{
    if(buf->current_line + 1 < buf->lines.count) {
        buf->cursor = buf->lines.items[buf->current_line + 1].end;
        buf->current_line += 1;
    }
}

void buffer_move_to_char_left(Buffer *buf)
{
    // TODO: we want move_to_char_left not working if it's the
    //       end of current line (just like VIM)
    if(buf->cursor != 0) {
        Line curr = buf->lines.items[buf->current_line];
        size_t prob = buf->cursor - 1;
        if(curr.start <= prob && prob <= curr.end) {
            buf->cursor = prob;
        }
    }
}

void buffer_move_to_start_of_line(Buffer *buf)
{
    Line curr = buf->lines.items[buf->current_line];
    buf->cursor = curr.start;
}

void buffer_move_to_end_of_line(Buffer *buf)
{
    Line curr = buf->lines.items[buf->current_line];
    buf->cursor = curr.end;
}

void buffer_move_to_char_right(Buffer *buf)
{
    // TODO: we want move_to_char_right not working if it's the
    //       start of current line (just like VIM)
    if(buf->cursor < buffer_length(buf)) {
        Line curr = buf->lines.items[buf->current_line];
        size_t prob = buf->cursor + 1;
        if(curr.start <= prob && prob <= curr.end) {
            buf->cursor = prob;
        }
    }
}
