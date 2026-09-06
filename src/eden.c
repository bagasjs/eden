#include "buffer.h"
#include "fontatlas.h"
#include "ren.h"
#include <stdlib.h>

#include "tp/stb_image.h"
#include "tp/glad.h"

#define RGFW_OPENGL
#define RGFW_PRINT_ERRORS
#define RGFW_IMPLEMENTATION
#include "tp/RGFW.h"

RenImage *ren_load_image_from_file(const char *filepath)
{
    int w, h, channels;
    stbi_uc* data = stbi_load(filepath, &w, &h, &channels, 0);

    RenImage *image = ren_load_image(data, w, h, channels);
    stbi_image_free(data);
    return image;
}

bool load_font_atlas_from_file(FontAtlas *atlas, const char *filepath)
{
    FILE* fontFile = fopen(filepath, "rb");
    if(!fontFile) {
        fprintf(stderr, "[ERROR] Failed to find font %s\n", filepath);
        return false;
    }

    fseek(fontFile, 0, SEEK_END);
    size_t size = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    uint8_t *fontBuffer = MALLOC_WITH_LABEL(size, "tmp.fontBuffer");
    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);
    if(!load_font_atlas(atlas, fontBuffer, 24, 0, NULL)) {
        return false;
    }
    FREE_WITH_LABEL(fontBuffer, "tmp.fontBuffer");
    return true;
}

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT,
    MODE_COMMAND,
} Mode;

typedef struct EditorConfig {
    RenColor background;

    size_t font_size;
    size_t tab_length;
    size_t scroll_margin;
    const char *font_filepath;
} EditorConfig;

typedef struct {
    EditorConfig config;

    Buffer *buf;
    rune prevc;

    Buffer *cmd;
    Mode mode;
    int scroll_line;

    FontAtlas *font;

    bool hide_cursor; // cursor ticker

    int window_width;
    int window_height;

    struct {
        char  *items;
        size_t count;
        size_t capacity;
    } tmp;

    bool exit;
} Editor;

#define CURSOR_WIDTH 2

void editor_render_buffer(Editor *e, Buffer *buf, int x, int y, size_t max_lines)
{
    int cx = x;
    int cy = y;

    // TODO: would be better if we do 60FPS
    // e->hide_cursor = !e->hide_cursor;

    if(buf->cursor == 0) {
        if(cx <= x) {
            if(!e->hide_cursor) {
                ren_draw_rect(
                        (RenRect) { .x = x, .y = cy, .w = CURSOR_WIDTH, .h = e->config.font_size }, 
                        REN_WHITE);
            }
        }
    }

    /*
     * Keep the cursor inside a scroll margin.
     *
     * scroll_line is persistent state in Editor.
     */
    size_t line_start = e->scroll_line;
    size_t line_end = line_start + max_lines;

    /*
     * Cursor moved above the upper scroll margin.
     */
    if (buf->current_line < line_start + e->config.scroll_margin) {
        if (buf->current_line >= e->config.scroll_margin)
            line_start = buf->current_line - e->config.scroll_margin;
        else
            line_start = 0;
    }

    /*
     * Cursor moved below the lower scroll margin.
     */
    if (buf->current_line >=
        line_start + max_lines - e->config.scroll_margin)
    {
        if (max_lines > e->config.scroll_margin) {
            line_start =
                buf->current_line
                - max_lines
                + e->config.scroll_margin
                + 1;
        } else {
            line_start = buf->current_line;
        }
    }

    /*
     * Don't scroll beyond the beginning/end of the buffer.
     */
    if (line_start > buf->lines.count)
        line_start = buf->lines.count;

    if (line_start + max_lines > buf->lines.count) {
        if (buf->lines.count > max_lines)
            line_start = buf->lines.count - max_lines;
        else
            line_start = 0;
    }

    /*
     * Store the calculated viewport position so the next
     * frame starts from the same position.
     */
    e->scroll_line = line_start;

    line_end = line_start + max_lines;

    if (line_end > buf->lines.count)
        line_end = buf->lines.count;

    for(size_t line_num = line_start; line_num < line_end; ++line_num) {
        Line line = buf->lines.items[line_num];

        for(size_t i = line.start; i < line.end; ++i) {
            rune c = buffer_getitem(buf, i);
            switch(c) {
                case '\n':
                    goto next_line;
                case '\t':
                    break;
                default:
                    cx = draw_codepoint(c, e->font, cx, cy, e->config.font_size, REN_WHITE);
                    break;
            }

            if(i + 1 == buf->cursor) {
                if(cx <= x) {
                    if(!e->hide_cursor) {
                        ren_draw_rect(
                                (RenRect) { .x = x, .y = cy, .w = CURSOR_WIDTH, .h = e->config.font_size }, 
                                REN_WHITE);
                    }
                } else {
                    if(!e->hide_cursor) {
                        ren_draw_rect(
                                (RenRect) { .x = cx - CURSOR_WIDTH, .y = cy, .w = CURSOR_WIDTH, .h = e->config.font_size }, 
                                REN_WHITE);
                    }
                }
            }
        }

next_line:
        cx = x;
        cy += e->config.font_size;
    }
}

void editor_render_statusbar(Editor *e)
{
    int padding = 0;

    /// File name bar
    RenRect namebar = {0};
    namebar.x = 0;
    namebar.y = e->window_height - 2*e->config.font_size - padding;
    namebar.w = e->window_width;
    namebar.h = e->config.font_size + padding;
    const char *file_path = e->buf->filepath;
    if(!file_path) file_path = "[No Name]";
    ren_draw_rect(namebar, REN_WHITE);
    draw_text(file_path, e->font, namebar.x, namebar.y, e->config.font_size, REN_BLACK);

    /// Command bar
    RenRect outer = {0};
    outer.x = 0;
    outer.y = e->window_height - e->config.font_size - padding;
    outer.w = e->window_width;
    outer.h = e->config.font_size + padding;
    ren_draw_rect(outer, REN_BLACK);

    switch(e->mode) {
        case MODE_INSERT:
            draw_text("-- INSERT --", e->font, outer.x, outer.y, e->config.font_size, REN_WHITE);
            break;
        case MODE_COMMAND:
            {
                int offset = 0;
                offset = draw_codepoint(':', e->font, outer.x + offset, outer.y, e->config.font_size, REN_WHITE);
                editor_render_buffer(e, e->cmd, outer.x + offset, outer.y, 1);
            } break;
        case MODE_NORMAL:
        default:
            break;
    }
}

void editor_render(Editor *e, int x, int y)
{
    editor_render_buffer(e, e->buf, x, y, e->window_height/e->config.font_size - 2);
    editor_render_statusbar(e);
}

#include <stdio.h>
void editor_handle_command(Editor *ed, StringView command)
{
    printf("%s\n", command.items);
    if(sv_eq(command, SVLIT("q"))) {
        ed->exit = true;
    } else if(sv_eq(command, SVLIT("w"))) {

    }
}

// TODO: Platform abstraction
//       We need a better platform abstraction layer.
//       Mostly we need a set of keycode, event type, etc.
void editor_handle_key_event(Editor *ed, int key)
{
    switch(key) {
        case RGFW_keyLeft:
            buffer_move_to_char_left(ed->buf);
            break;
        case RGFW_keyRight:
            buffer_move_to_char_right(ed->buf);
            break;
        case RGFW_keyUp:
            buffer_move_to_line_above(ed->buf);
            break;
        case RGFW_keyDown:
            buffer_move_to_line_below(ed->buf);
            break;
        default:
            break;
    }
}

// TODO: Platform abstraction
//       We need a better platform abstraction layer.
//       Mostly we need a set of keycode, event type, etc.
void editor_handle_keychar_event(Editor *ed, rune c)
{
#define BACKSPACE 8
#define TAB 9
#define ENTER 13
#define ESCAPE 27

    if(ed->mode == MODE_COMMAND) {
        switch(c) {
            case ENTER:
                editor_handle_command(ed, buffer_to_sv(ed->cmd));
                buffer_reset(ed->cmd);
                ed->mode = MODE_NORMAL;
                break;
            case ESCAPE:
                buffer_reset(ed->cmd);
                ed->mode = MODE_NORMAL;
                break;
            case BACKSPACE:
                buffer_backspace(ed->cmd);
                break;
            default: 
                buffer_insert_char(ed->cmd, c);
                break;
        }
    }

    if(ed->mode == MODE_INSERT) {
        switch(c) {
            case TAB:
                for(size_t i = 0; i < ed->config.tab_length; ++i)
                    buffer_insert_char(ed->buf, ' ');
                break;
            case ENTER:
                buffer_insert_char(ed->buf, '\n');
                break;
            case BACKSPACE:
                buffer_backspace(ed->buf);
                break;
            case ESCAPE:
                ed->mode = MODE_NORMAL;
                break;
            default: 
                buffer_insert_char(ed->buf, c);
                // printf("KEYCODE: %u CHAR: '%c'\n", c, c);
                break;
        }
    }

    if(ed->mode == MODE_NORMAL) {
        switch(c) {
            case '=':
                buffer__debug(ed->buf);
                break;
            case 'd':
                if(ed->prevc == 'd') buffer_delete_current_line(ed->buf);
                break;
            case 'g':
                if(ed->prevc == 'g') buffer_move_to_first_line(ed->buf);
                break;
            case 'G':
                buffer_move_to_last_line(ed->buf);
                break;
            case '0':
                buffer_move_to_start_of_line(ed->buf);
                break;
            case '$':
            case '-':
                buffer_move_to_end_of_line(ed->buf);
                break;
            case 'i':
                ed->mode = MODE_INSERT;
                break;
            case 'a':
                buffer_move_to_char_right(ed->buf);
                ed->mode = MODE_INSERT;
                break;
            case ':':
                ed->mode = MODE_COMMAND;
                break;
            case 'h':
                buffer_move_to_char_left(ed->buf);
                break;
            case 'l':
                buffer_move_to_char_right(ed->buf);
                break;
            case 'j':
                buffer_move_to_line_below(ed->buf);
                break;
            case 'k':
                buffer_move_to_line_above(ed->buf);
                break;
            case 'o':
                buffer_move_to_end_of_line(ed->buf);
                buffer_insert_char(ed->buf, '\n');
                ed->mode = MODE_INSERT;
                break;
            case 'O':
                buffer_move_to_line_above(ed->buf);
                buffer_insert_char(ed->buf, '\n');
                ed->mode = MODE_INSERT;
                break;
            default:
                break;
        }
    }

    ed->prevc = c;
}

#define NOB_IMPLEMENTATION
#include "nob.h"
void buffer_insert_file_content(Buffer *buffer, const char *file_path)
{
    Nob_String_Builder sb = {0};
    if(!nob_read_entire_file(file_path, &sb)) {
        // TODO: we want the editor's command buffer show this error message
        fprintf(stderr, "ERR: failed to load file %s\n", file_path);
    }
    buffer_insert(buffer, sb.items, sb.count);
    sb_free(sb);
}

EditorConfig parse_config_file(const char *file_path)
{
    EditorConfig config = {0};
    config.font_size  = 20;
    config.tab_length = 4;
    config.background = (RenColor){ 0x18, 0x36, 0x48, 0xFF };
    config.font_filepath = "./assets/firacode.ttf";
    config.scroll_margin = 3;
    return config;
}

int main(int argc, char *argv[])
{
    EditorConfig config = parse_config_file(NULL);

    const char *file_path = NULL;
    if(argc > 1) {
        file_path = argv[1];
    }

    RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
    // NOTE: We use 4.3 in because in Windows glDebugMessageCallback is not exists in 3.3
    hints->major = 4;
    hints->minor = 3;
    RGFW_setGlobalHints_OpenGL(hints);

    RGFW_window *window = RGFW_createWindow("Eden", 0, 0, 800, 600, 
        RGFW_windowAllowDND | RGFW_windowCenter | RGFW_windowScaleToMonitor | RGFW_windowOpenGL);

    if(!window) {
        fprintf(stderr, "[ERROR] failed to open window\n");
        return -1;
    }

    RGFW_window_makeCurrentContext_OpenGL(window);

    if(!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
        fprintf(stderr, "[ERROR] failed to load OpenGL functions\n");
        return -1;
    }

    int w, h;
    RGFW_window_getSize(window, &w, &h);

    ren_init();
    ren_viewport(0, 0, w, h);


    Editor ed = {0};
    ed.config = config;

    FontAtlas atlas = {0};
    if(!load_font_atlas_from_file(&atlas, config.font_filepath)) {
        return -1;
    }

    ed.cmd  = buffer_new();
    ed.buf  = buffer_new();
    ed.font = &atlas;
    ed.exit = false;

    if(file_path) {
        buffer_insert_file_content(ed.buf, file_path);
        ed.buf->filepath = file_path;
        printf("Loaded file: %s\n", file_path);
        printf("  Total lines: %zu\n", ed.buf->lines.count);
        printf("  Current Line: %zu\n", ed.buf->current_line);
    }

    while(!ed.exit && RGFW_window_shouldClose(window) == RGFW_FALSE) {
        ed.window_width  = window->w;
        ed.window_height = window->h;

        RGFW_event event;
        while (RGFW_window_checkEvent(window, &event)) {
            if (event.type == RGFW_windowClose) {
                ed.exit = true;
            }

            switch(event.type) {
            case RGFW_keyChar:
                {
                    editor_handle_keychar_event(&ed, event.keyChar.value);
                } break;
            case RGFW_keyPressed:
                {
                    switch(event.key.value) {
                    case RGFW_keyLeft:
                    case RGFW_keyRight:
                    case RGFW_keyUp:
                    case RGFW_keyDown:
                        editor_handle_key_event(&ed, event.key.value);
                        break;
                    default:
                        break;
                    }
                } break;
            default:
                break;
            }
        }

        ren_clear(ed.config.background);
        editor_render(&ed, 0, 0);
        ren_flush();

        RGFW_window_swapBuffers_OpenGL(window);
    }

    buffer_destroy(ed.buf);
    buffer_destroy(ed.cmd);

    unload_font_atlas(&atlas);
    ren_deinit();
    RGFW_window_close(window);
    return 0;
}
