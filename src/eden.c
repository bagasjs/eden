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

typedef struct EditorCore {
    Buffer *buf;
    Buffer *cmd;
    Mode    mode;
    rune    prevc;
} EditorCore;

typedef struct EditorUI {
    FontAtlas *font;
    int window_width;
    int window_height;

    int prev_line_start;
} EditorUI;

typedef struct {
    EditorCore   core;
    EditorConfig config;
    EditorUI     ui;
    bool hide_cursor; // cursor ticker
    bool exit;
} Editor;

#define CURSOR_WIDTH 2

void editor_ui_handle_y_overflow(Editor *ed, Buffer *buf, int *output_line_start, int *output_line_end, int max_lines)
{
    size_t line_start = ed->ui.prev_line_start;
    if(buf->current_line < line_start + ed->config.scroll_margin) {
        line_start = buf->current_line >= ed->config.scroll_margin ?
            buf->current_line - ed->config.scroll_margin :
            0;
    }
    if(buf->current_line >= line_start + max_lines - ed->config.scroll_margin) {
        if(max_lines > ed->config.scroll_margin) {
            line_start =
                buf->current_line
                - max_lines
                + ed->config.scroll_margin
                + 1;
        } else {
            line_start = buf->current_line;
        }
    }

    if (line_start > buf->lines.count) line_start = buf->lines.count;

    ed->ui.prev_line_start = line_start;

    size_t line_end = line_start + max_lines;
    if (line_end > buf->lines.count) line_end = buf->lines.count;
    
    *output_line_start = line_start;
    *output_line_end   = line_end;
}

void editor_ui_render_buffer(Editor *ed, Buffer *buf, int x, int y, int width, int height)
{
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x + width  > ed->ui.window_width)  width  = ed->ui.window_width  - x;
    if(y + height > ed->ui.window_height) height = ed->ui.window_height - y;

    int cx = x;
    int cy = y;

    int line_start, line_end;
    editor_ui_handle_y_overflow(ed, buf, &line_start, &line_end, height/ed->config.font_size);

    for(int line_num = line_start; line_num < line_end; ++line_num) {
        Line line = buf->lines.items[line_num];
        if(buf->cursor == line.start) {
            ren_draw_rect(
                    (RenRect){ .x = x, .y = cy, .w = CURSOR_WIDTH, .h = ed->config.font_size },
                    REN_WHITE);
        }

        for(size_t i = line.start; i <= line.end; ++i) {
            rune c = buffer_getitem(buf, i);
            switch(c) {
                case '\n':
                    goto next_line;
                case '\t':
                    break;
                default:
                    cx = draw_codepoint(c, ed->ui.font, cx, cy, ed->config.font_size, REN_WHITE);
                    break;
            }
            if(buf->cursor == i + 1) {
                ren_draw_rect(
                        (RenRect){ .x = cx, .y = cy, .w = CURSOR_WIDTH, .h = ed->config.font_size },
                        REN_WHITE);
            }
        }
        next_line:
        cx = x;
        cy += ed->config.font_size;
    }
}

void editor_ui_render_statusbar(Editor *e)
{
    int padding = 0;

    /// File name bar
    RenRect namebar = {0};
    namebar.x = 0;
    namebar.y = e->ui.window_height - 2*e->config.font_size - padding;
    namebar.w = e->ui.window_width;
    namebar.h = e->config.font_size + padding;
    const char *file_path = e->core.buf->filepath;
    if(!file_path) file_path = "[No Name]";
    ren_draw_rect(namebar, REN_WHITE);
    draw_text(file_path, e->ui.font, namebar.x, namebar.y, e->config.font_size, REN_BLACK);

    /// Command bar
    RenRect outer = {0};
    outer.x = 0;
    outer.y = e->ui.window_height - e->config.font_size - padding;
    outer.w = e->ui.window_width;
    outer.h = e->config.font_size + padding;
    ren_draw_rect(outer, REN_BLACK);

    switch(e->core.mode) {
        case MODE_INSERT:
            draw_text("-- INSERT --", e->ui.font, outer.x, outer.y, e->config.font_size, REN_WHITE);
            break;
        case MODE_COMMAND:
            {
                int offset = 0;
                offset = draw_codepoint(':', e->ui.font, outer.x + offset, outer.y, e->config.font_size, REN_WHITE);
                editor_ui_render_buffer(e, e->core.cmd, outer.x + offset, outer.y, e->ui.window_width, e->config.font_size);
            } break;
        case MODE_NORMAL:
        default:
            break;
    }
}

void editor_ui_render(Editor *ed, int x, int y)
{
    editor_ui_render_buffer(ed, ed->core.buf, 0, 0, ed->ui.window_width, ed->ui.window_height);
    /*editor_render_buffer(e, e->core.buf, x, y, e->ui.window_height/e->config.font_size - 2);*/
    editor_ui_render_statusbar(ed);
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
            buffer_move_to_char_left(ed->core.buf);
            break;
        case RGFW_keyRight:
            buffer_move_to_char_right(ed->core.buf);
            break;
        case RGFW_keyUp:
            buffer_move_to_line_above(ed->core.buf);
            break;
        case RGFW_keyDown:
            buffer_move_to_line_below(ed->core.buf);
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

    if(ed->core.mode == MODE_COMMAND) {
        switch(c) {
            case ENTER:
                editor_handle_command(ed, buffer_to_sv(ed->core.cmd));
                buffer_reset(ed->core.cmd);
                ed->core.mode = MODE_NORMAL;
                break;
            case ESCAPE:
                buffer_reset(ed->core.cmd);
                ed->core.mode = MODE_NORMAL;
                break;
            case BACKSPACE:
                buffer_backspace(ed->core.cmd);
                break;
            default: 
                buffer_insert_char(ed->core.cmd, c);
                break;
        }
    }

    if(ed->core.mode == MODE_INSERT) {
        switch(c) {
            case TAB:
                for(size_t i = 0; i < ed->config.tab_length; ++i)
                    buffer_insert_char(ed->core.buf, ' ');
                break;
            case ENTER:
                buffer_insert_char(ed->core.buf, '\n');
                break;
            case BACKSPACE:
                buffer_backspace(ed->core.buf);
                break;
            case ESCAPE:
                ed->core.mode = MODE_NORMAL;
                break;
            default: 
                buffer_insert_char(ed->core.buf, c);
                // printf("KEYCODE: %u CHAR: '%c'\n", c, c);
                break;
        }
    }

    if(ed->core.mode == MODE_NORMAL) {
        switch(c) {
            case '=':
                buffer__debug(ed->core.buf);
                break;
            case 'd':
                if(ed->core.prevc == 'd') buffer_delete_current_line(ed->core.buf);
                break;
            case 'g':
                if(ed->core.prevc == 'g') buffer_move_to_first_line(ed->core.buf);
                break;
            case 'G':
                buffer_move_to_last_line(ed->core.buf);
                break;
            case '0':
                buffer_move_to_start_of_line(ed->core.buf);
                break;
            case '$':
            case '-':
                buffer_move_to_end_of_line(ed->core.buf);
                break;
            case 'i':
                ed->core.mode = MODE_INSERT;
                break;
            case 'a':
                buffer_move_to_char_right(ed->core.buf);
                ed->core.mode = MODE_INSERT;
                break;
            case ':':
                ed->core.mode = MODE_COMMAND;
                break;
            case 'h':
                buffer_move_to_char_left(ed->core.buf);
                break;
            case 'l':
                buffer_move_to_char_right(ed->core.buf);
                break;
            case 'j':
                buffer_move_to_line_below(ed->core.buf);
                break;
            case 'k':
                buffer_move_to_line_above(ed->core.buf);
                break;
            case 'o':
                buffer_move_to_end_of_line(ed->core.buf);
                buffer_insert_char(ed->core.buf, '\n');
                ed->core.mode = MODE_INSERT;
                break;
            case 'O':
                buffer_move_to_line_above(ed->core.buf);
                buffer_insert_char(ed->core.buf, '\n');
                ed->core.mode = MODE_INSERT;
                break;
            default:
                break;
        }
    }

    ed->core.prevc = c;
}

#define NOB_IMPLEMENTATION
#define NOB_UNSTRIP_PREFIX
#include "nob.h"
void buffer_insert_file_content(Buffer *buffer, const char *file_path)
{
    Nob_String_Builder sb = {0};
    if(!nob_read_entire_file(file_path, &sb)) {
        // TODO: we want the editor's command buffer show this error message
        fprintf(stderr, "ERR: failed to load file %s\n", file_path);
    }
    buffer_insert(buffer, sb.items, sb.count);
    nob_sb_free(sb);
}

EditorConfig parse_config_file(const char *file_path)
{
    (void)file_path;
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

    ed.core.cmd  = buffer_new();
    ed.core.buf  = buffer_new();
    ed.ui.font = &atlas;
    ed.exit = false;

    if(file_path) {
        buffer_insert_file_content(ed.core.buf, file_path);
        ed.core.buf->filepath = file_path;
        printf("Loaded file: %s\n", file_path);
        printf("  Total lines: %zu\n", ed.core.buf->lines.count);
        printf("  Current Line: %zu\n", ed.core.buf->current_line);
    }

    while(!ed.exit && RGFW_window_shouldClose(window) == RGFW_FALSE) {
        ed.ui.window_width  = window->w;
        ed.ui.window_height = window->h;

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
        editor_ui_render(&ed, 0, 0);
        ren_flush();

        RGFW_window_swapBuffers_OpenGL(window);
    }

    buffer_destroy(ed.core.buf);
    buffer_destroy(ed.core.cmd);

    unload_font_atlas(&atlas);
    ren_deinit();
    RGFW_window_close(window);
    return 0;
}
