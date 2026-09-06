#include "buffer.h"
#include "common.h"
#include <string.h>
#include <stdio.h>

void buffer__debug(Buffer *buf)
{
    printf("Cursor: %zu\n", buf->cursor);
    printf("Current Line: %zu\n", buf->current_line);
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

void buffer_reset(Buffer *buffer)
{
    buffer->lines.count = 0;
    buffer->cursor = 0;
    buffer->current_line = 0;
    buffer_unsafe_reset(buffer);
}

void buffer_move_cursor_to(Buffer *buf, size_t index)
{
    // NOTE: move cursor will commit a transaction for undo operation
    buf->cursor = index;
}

void buffer_move_cursor_to_line(Buffer *buf, size_t line_number, size_t line_offset)
{
    if(line_number > buf->lines.count) {
        line_number = buf->lines.count - 1;
    }
    Line line = buf->lines.items[line_number];
    size_t line_len = line.end - line.start + 1;
    if(line_offset > line_len) {
        line_offset = line_len - 1;
    }
    size_t index = line.start + line_offset;

    buffer_move_cursor_to(buf, index);
    buf->current_line = line_number;
}


void buffer_move_cursor_to_left(Buffer *buf, size_t n_step)
{
    /*printf("MOVE TO LEFT:\n");*/
    size_t prob = buf->cursor - n_step;
    if(buf->cursor < n_step) prob = 0;
    size_t curr_line_num = buf->current_line;
    Line   curr_line     = buf->lines.items[curr_line_num];
    /*printf("    current: %zu\n", buf->cursor);*/
    /*printf("    target: %zu\n",  prob);*/
    /*printf("    Probing line\n");*/
    /*printf("        current line: %zu\n", curr_line_num);*/
    while(curr_line_num != 0 && prob < curr_line.start) {
        curr_line_num -= 1;
        curr_line      = buf->lines.items[curr_line_num];
        /*printf("        previous line: %zu [%zu - %zu]\n", curr_line_num, curr_line.start, curr_line.end);*/
    }

    buffer_move_cursor_to(buf, prob);
    buf->current_line = curr_line_num;
}

void buffer_move_cursor_to_right(Buffer *buf, size_t n_step)
{
    printf("MOVE TO RIGHT:\n");
    size_t length = buffer_length(buf);
    size_t prob = buf->cursor + n_step;
    if(buf->cursor + n_step >= length) prob = length;
    size_t curr_line_num = buf->current_line;
    Line   curr_line     = buf->lines.items[curr_line_num];
    printf("    current: %zu\n", buf->cursor);
    printf("    target: %zu\n",  prob);
    printf("    Probing line\n");
    printf("        current line: %zu\n", curr_line_num);
    while(curr_line_num < length && prob > curr_line.end) {
        curr_line_num += 1;
        curr_line      = buf->lines.items[curr_line_num];
        printf("        next line: %zu [%zu - %zu]\n", curr_line_num, curr_line.start, curr_line.end);
    }
    buffer_move_cursor_to(buf, prob);
    buf->current_line = curr_line_num;
}

void buffer_insert(Buffer *buf, const char *text, size_t size)
{
    buffer_unsafe_insert(buf, buf->cursor, text, size);
    buffer_update_lines(buf);
    buffer_move_cursor_to_right(buf, size);
}

void buffer_insert_char(Buffer *buf, rune ch)
{
    buffer_insert(buf, (char*)&ch, 1);
}

void buffer_backspace(Buffer *buf)
{
    if(buf->cursor == 0) return;
    buffer_unsafe_delete(buf, buf->cursor - 1, 1);
    buffer_move_cursor_to_left(buf, 1);
    buffer_update_lines(buf);
}

void buffer_delete_current_line(Buffer *buf)
{
    Line current = buf->lines.items[buf->current_line];
    buffer_unsafe_delete(buf, current.start, current.end - current.start + 1);
    buffer_update_lines(buf);
}

void buffer_move_to_line_above(Buffer *buf)
{
    // NOTE: -1 for the last parameter which is the line_offset works because
    //       in C -1 for uint is casted into UINT_MAX
    // TODO: Taking advantage of UB
    //       This feels wrong. Find a better way to do this
    if(buffer_get_current_line(buf) != 0) 
        buffer_move_cursor_to_line(buf, buf->current_line - 1, -1);
}

void buffer_move_to_line_below(Buffer *buf)
{
    // NOTE: -1 for the last parameter which is the line_offset works because
    //       in C -1 for uint is casted into UINT_MAX
    // TODO: Taking advantage of UB
    //       This feels wrong. Find a better way to do this
    if(buffer_get_current_line(buf) + 1 < buffer_get_total_lines(buf)) 
        buffer_move_cursor_to_line(buf, buf->current_line + 1, -1);
}

/*void buffer_move_to_char_left(Buffer *buf)*/
/*{*/
/*    buffer_move_cursor_to_left(buf, 1);*/
/*}*/

void buffer_move_to_char_left(Buffer *buf)
{
    // TODO: we want move_to_char_left not working if it's the
    //       end of current line (just like VIM)
    buffer_move_cursor_to_left(buf, 1);
}

void buffer_move_to_char_right(Buffer *buf)
{
    // TODO: we want move_to_char_right not working if it's the
    //       start of current line (just like VIM)
    buffer_move_cursor_to_right(buf, 1);
    /*if(buf->cursor < buffer_length(buf)) {*/
    /*    Line curr = buf->lines.items[buf->current_line];*/
    /*    size_t prob = buf->cursor + 1;*/
    /*    if(curr.start <= prob && prob <= curr.end) {*/
    /*        buf->cursor = prob;*/
    /*    }*/
    /*}*/
}

void buffer_move_to_start_of_line(Buffer *buf)
{
    buffer_move_cursor_to_line(buf, buffer_get_current_line(buf), 0);
}

void buffer_move_to_end_of_line(Buffer *buf)
{
    // NOTE: -1 for the last parameter which is the line_offset works because
    //       in C -1 for uint is casted into UINT_MAX
    // TODO: Taking advantage of UB
    //       This feels wrong. Find a better way to do this
    buffer_move_cursor_to_line(buf, buffer_get_current_line(buf), -1);
}

void buffer_move_to_first_line(Buffer *buf)
{
    buffer_move_cursor_to_line(buf, 0, -1);
}

void buffer_move_to_last_line(Buffer *buf)
{
    buffer_move_cursor_to_line(buf, buf->lines.count - 1, -1);
}
