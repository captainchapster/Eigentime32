/* 1-bit framebuffer drawing for the 128 x 128 Sharp memory LCD. Portable C. */
#ifndef GFX_H_INCLUDED
#define GFX_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#define GFX_W 128
#define GFX_H 128
#define GFX_STRIDE (GFX_W / 8)

/* Row-major, 16 bytes per row, pixel x in bit (x & 7) of byte x / 8 (the
 * LCD's LSB-first order). 1 = black. */
typedef struct {
    uint8_t px[GFX_H][GFX_STRIDE];
} gfx_fb_t;

typedef struct {
    char ch;
    uint8_t width;
    uint16_t offset;
} font_glyph_t;

typedef struct {
    uint8_t height;
    uint8_t count;
    const font_glyph_t *glyphs;
    const uint8_t *bits;
} font_t;

extern const font_t font_big;
extern const font_t font_small;

void gfx_clear(gfx_fb_t *fb);
void gfx_pixel(gfx_fb_t *fb, int x, int y, bool on);
void gfx_fill_rect(gfx_fb_t *fb, int x, int y, int w, int h, bool on);
void gfx_invert_rect(gfx_fb_t *fb, int x, int y, int w, int h);
void gfx_fill_circle(gfx_fb_t *fb, int cx, int cy, int r, bool on);
/* Radial line from radius r0 to r1 at angle a (0..3599 tenths of a degree, 0 = 12 o'clock, clockwise). */
void gfx_radial(gfx_fb_t *fb, int cx, int cy, int r0, int r1, int a, int thick);
int gfx_text_width(const font_t *f, const char *s);
/* Draws s with its top-left at (x, y); returns the width drawn. */
int gfx_text(gfx_fb_t *fb, const font_t *f, int x, int y, const char *s);
/* Draws s centred on cx; returns the x of its left edge. */
int gfx_text_centered(gfx_fb_t *fb, const font_t *f, int cx, int y, const char *s);

#endif
