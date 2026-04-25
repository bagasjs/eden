#include "buffer.h"
#include "fontatlas.h"
#include "ren.h"
#include <stdlib.h>

#define RGFW_OPENGL
#define RGFW_IMPLEMENTATION
#include "tp/RGFW.h"
#define STB_IMAGE_IMPLEMENTATION
#include "tp/stb_image.h"

#include "tp/glad.h"

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
    fseek(fontFile, 0, SEEK_END);
    size_t size = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);
    uint8_t *fontBuffer = malloc(size);
    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);
    if(!load_font_atlas(atlas, fontBuffer, 64, 0, NULL)) {
        return false;
    }
    free(fontBuffer);
    return true;
}

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT,
} Mode;

typedef struct {
    Buffer *buf;
    Mode mode;

    FontAtlas *font;
    size_t font_size;
    size_t tab_length;
} Editor;

void editor_render(Editor *e, int x, int y)
{
    int cx = x;
    int cy = y;

    if(e->buf->cursor == 0) {
        if(cx <= x) {
            const int width = 2;
            ren_draw_rect((RenRect) { .x = 0, .y = cy, .w = width, .h = e->font_size }, REN_WHITE);
        }
    }

    for(size_t i = 0; i < buffer_length(e->buf); ++i) {
        rune c = buffer_getitem(e->buf, i);

        switch(c) {
            case '\n':
                cx = x;
                cy += e->font_size;
                break;
            case '\t':
                break;
            default:
                cx = draw_codepoint(c, e->font, cx, cy, e->font_size, REN_WHITE);
                break;
        }

        if(i + 1 == e->buf->cursor) {
            if(cx <= x) {
                const int width = 2;
                ren_draw_rect((RenRect) { .x = 0, .y = cy, .w = width, .h = e->font_size }, REN_WHITE);
            } else {
                const int width = 2;
                ren_draw_rect((RenRect) { .x = cx - width, .y = cy, .w = width, .h = e->font_size }, REN_WHITE);
            }
        }
    }
}

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
            {
                buffer_dump(ed->buf);
            } break;
        default:
            break;
    }
}

void editor_handle_keychar_event(Editor *ed, rune c)
{
#define BACKSPACE 8
#define TAB 9
#define ENTER 13
#define ESCAPE 27

    if(ed->mode == MODE_INSERT) {
        switch(c) {
            case TAB:
                for(size_t i = 0; i < ed->tab_length; ++i)
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
            case 'i':
                ed->mode = MODE_INSERT;
                break;
            case 'h':
                buffer_move_to_char_left(ed->buf);
                break;
            case 'l':
                buffer_move_to_char_right(ed->buf);
                break;
            case 'j':
                break;
            case 'k':
                break;
            default:
                break;
        }
    }
}

int main(void)
{
    RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 3;
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


    FontAtlas atlas = {0};
    if(!load_font_atlas_from_file(&atlas, "./assets/firacode.ttf")) {
        return -1;
    }

    Editor ed = {0};
    ed.buf  = buffer_new();
    ed.font = &atlas;
    ed.font_size  = 16;
    ed.tab_length = 4;

    while(RGFW_window_shouldClose(window) == RGFW_FALSE) {
        RGFW_event event;
        while (RGFW_window_checkEvent(window, &event)) {
            if (event.type == RGFW_windowClose) {
                break;
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

        ren_clear((RenColor){ 0x18, 0x36, 0x48, 0xFF });
        editor_render(&ed, 0, 0);
        ren_flush();

        RGFW_window_swapBuffers_OpenGL(window);
    }

    buffer_destroy(ed.buf);

    unload_font_atlas(&atlas);
    ren_deinit();
    RGFW_window_close(window);
    return 0;
}

