#ifndef REN_H_
#define REN_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define REN_WHITE (RenColor){ 0xFF, 0xFF, 0xFF, 0xFF }
#define REN_BLACK (RenColor){ .a = 0xFF }

typedef struct { int x, y, w, h; } RenRect;
typedef struct { uint8_t r, g, b, a; } RenColor;

typedef struct RenImage RenImage;

bool ren_init(void);
void ren_deinit(void);
void ren_viewport(int x, int y, int width, int height);

// TODO: it's would be more explicit if we change n_channels to pixel_format
//       something like RGBA, RGB and GREYSCALE
RenImage *ren_load_image(const void *pixels, int width, int height, int n_channels);
void ren_unload_image(RenImage *image);
RenRect ren_get_image_rect(RenImage *image);

void ren_clear(RenColor color);
void ren_flush(void);
void ren_draw_rect(RenRect r, RenColor color);
void ren_draw_image(RenImage *image, RenRect src, RenRect dst, RenColor color);

#endif // REN_H_
