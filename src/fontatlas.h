#ifndef FONTATLAS_H_
#define FONTATLAS_H_

#include "ren.h"
#include <stdint.h>

typedef struct {
    int codepoint;
    float s1, t1, s2, t2;
} GlyphMetric;

typedef struct {
    RenImage *image;
    uint32_t width;
    uint32_t height;
    GlyphMetric *chars;
    int count_chars;
} FontAtlas;

bool load_font_atlas(FontAtlas *atlas, const uint8_t *font_data, int base_font_size, int codepoint_amount, int *codepoints);
void unload_font_atlas(FontAtlas *atlas);
int draw_codepoint(int codepoint, FontAtlas *atlas, int x, int y, int font_size, RenColor tint);

#endif // FONTATLAS_H_
