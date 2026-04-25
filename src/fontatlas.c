#include "fontatlas.h"
#include "ren.h"
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "tp/stb_truetype.h"


bool load_font_atlas(FontAtlas *atlas, const uint8_t *font_data, int base_font_size, int codepoint_amount, int *codepoints)
{
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, font_data, 0)) {
        fprintf(stderr, "[ERROR] Failed to load font info\n");
        return false;
    }

    codepoint_amount = codepoint_amount != 0 ? codepoint_amount : 95;
    int is_codepoint_generated = 0;
    if(!codepoints) {
        is_codepoint_generated = 1;
        codepoints = malloc(sizeof(int) * codepoint_amount);
        for(int i = 0; i < codepoint_amount; ++i)
            codepoints[i] = 32 + i;
    }

    float scale = stbtt_ScaleForPixelHeight(&info, base_font_size);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    int x = 0;
    int y = 0;

    int bh = base_font_size;
    int bw = 0;
    // Get the width of the atlas
    for(int i = 0; i < codepoint_amount; ++i) {
        int codepoint = codepoints[i];
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&info, codepoint, &ax, &lsb);
        bw += roundf(ax * scale);
    }

    uint8_t *bitmap = malloc(bw * bh);
    memset(bitmap, 0, bw * bh);
    GlyphMetric *chars = malloc(sizeof(GlyphMetric) * codepoint_amount);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    for(int i = 0; i < codepoint_amount; ++i) {
        /* how wide is this character */
        int codepoint = codepoints[i];
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&info, codepoint, &ax, &lsb);
        /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character codepoint.) */

        /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, codepoint, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        /* compute y (different characters have different heights) */
        y = ascent + c_y1;
        
        /* render character (stride and offset is important here) */
        int byteOffset = x + roundf(lsb * scale) + (y * bw);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bw, scale, scale, codepoint);

        chars[i].codepoint = codepoint;
        chars[i].s1 = x;
        chars[i].t1 = 0.0f;

        /* advance x */
        x += roundf(ax * scale);
        chars[i].s2 = x;
        chars[i].t2 = base_font_size;
    }

    if(is_codepoint_generated)
        free(codepoints);

    
    uint8_t *data = (uint8_t *)malloc(bw*bh*4);
    for (int i = 0, k = 0; i < (int)(bw*bh); i++, k += 4) {
        data[k + 0] = 255;
        data[k + 1] = 255;
        data[k + 2] = 255;
        data[k + 3] = ((uint8_t *)bitmap)[i];
    }
    free(bitmap);

    atlas->width  = bw;
    atlas->height = bh;
    atlas->chars = chars;
    atlas->count_chars = codepoint_amount;
    atlas->image = ren_load_image(data, bw, bh, 4);
    free(data);
    return true;
}

void unload_font_atlas(FontAtlas *atlas)
{
    ren_unload_image(atlas->image);
    free(atlas->chars);
}

static GlyphMetric find_glyph_metric(FontAtlas *atlas, int codepoint)
{
    for(int i = 0; i < atlas->count_chars; ++i) {
        if(atlas->chars[i].codepoint == codepoint) {
            return atlas->chars[i];
        }
    }
    return atlas->chars[0];
}


int draw_codepoint(int codepoint, FontAtlas *atlas, int x, int y, int font_size, RenColor tint)
{
    float scale = (float)font_size/(float)atlas->height;
    GlyphMetric info = find_glyph_metric(atlas, codepoint);

    RenRect src;
    src.x = info.s1;
    src.y = info.t1;
    src.w = (float)info.s2 - info.s1;
    src.h = (float)info.t2 - info.t1;

    RenRect dst;
    dst.x = x;
    dst.y = y;
    dst.w = src.w*scale;
    dst.h = atlas->height*scale;
    ren_draw_image(atlas->image, src, dst, tint);
    return x + dst.w;
}
