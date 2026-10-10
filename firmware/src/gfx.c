#include "gfx.h"

#include <string.h>

#define SPACING 2

void gfx_clear(gfx_fb_t *fb) { memset(fb, 0, sizeof(*fb)); }

void gfx_pixel(gfx_fb_t *fb, int x, int y, bool on)
{
    if (x < 0 || y < 0 || x >= GFX_W || y >= GFX_H)
        return;
    uint8_t m = (uint8_t)(1u << (x & 7));
    if (on)
        fb->px[y][x >> 3] |= m;
    else
        fb->px[y][x >> 3] &= (uint8_t)~m;
}

void gfx_fill_rect(gfx_fb_t *fb, int x, int y, int w, int h, bool on)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            gfx_pixel(fb, i, j, on);
}

void gfx_invert_rect(gfx_fb_t *fb, int x, int y, int w, int h)
{
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) {
            if (i < 0 || j < 0 || i >= GFX_W || j >= GFX_H)
                continue;
            fb->px[j][i >> 3] ^= (uint8_t)(1u << (i & 7));
        }
}

void gfx_fill_circle(gfx_fb_t *fb, int cx, int cy, int r, bool on)
{
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx * dx + dy * dy <= r * r)
                gfx_pixel(fb, cx + dx, cy + dy, on);
}

/* sin of tenths of a degree, scaled by 1024, from a quarter-wave table in 1.5 degree steps */
static const int16_t sin_q[61] = {
    0, 27, 54, 80, 107, 134, 160, 187, 213, 239, 265, 291, 316, 342, 367, 392,
    416, 441, 465, 489, 512, 535, 558, 580, 602, 623, 644, 665, 685, 705, 724, 743,
    761, 779, 796, 812, 828, 844, 859, 873, 887, 900, 912, 924, 935, 946, 956, 965,
    974, 982, 989, 996, 1002, 1007, 1011, 1015, 1018, 1021, 1023, 1024, 1024,
};

static int isin(int a) /* a in tenths of a degree */
{
    a %= 3600;
    if (a < 0)
        a += 3600;
    int sign = 1;
    if (a >= 1800) {
        a -= 1800;
        sign = -1;
    }
    if (a > 900)
        a = 1800 - a;
    int i = a / 15, f = a % 15;
    int v = sin_q[i] + (i < 60 ? (sin_q[i + 1] - sin_q[i]) * f / 15 : 0);
    return sign * v;
}

void gfx_radial(gfx_fb_t *fb, int cx, int cy, int r0, int r1, int a, int thick)
{
    int s = isin(a), c = isin(a + 900);
    for (int r = r0 * 4; r <= r1 * 4; r++) {
        int x = cx + (s * r + 2048) / 4096;
        int y = cy - (c * r + 2048) / 4096;
        if (thick > 1)
            gfx_fill_circle(fb, x, y, thick / 2, true);
        else
            gfx_pixel(fb, x, y, true);
    }
}

static const font_glyph_t *find(const font_t *f, char ch)
{
    for (int i = 0; i < f->count; i++)
        if (f->glyphs[i].ch == ch)
            return &f->glyphs[i];
    return 0;
}

int gfx_text_width(const font_t *f, const char *s)
{
    int w = 0;
    for (; *s; s++) {
        const font_glyph_t *g = find(f, *s);
        if (g)
            w += g->width + SPACING;
    }
    return w ? w - SPACING : 0;
}

int gfx_text(gfx_fb_t *fb, const font_t *f, int x, int y, const char *s)
{
    int x0 = x;
    for (; *s; s++) {
        const font_glyph_t *g = find(f, *s);
        if (!g)
            continue;
        int stride = (g->width + 7) / 8;
        const uint8_t *p = f->bits + g->offset;
        for (int j = 0; j < f->height; j++)
            for (int i = 0; i < g->width; i++)
                if (p[j * stride + i / 8] & (1u << (i & 7)))
                    gfx_pixel(fb, x + i, y + j, true);
        x += g->width + SPACING;
    }
    return x > x0 ? x - x0 - SPACING : 0;
}

int gfx_text_centered(gfx_fb_t *fb, const font_t *f, int cx, int y, const char *s)
{
    int x = cx - gfx_text_width(f, s) / 2;
    gfx_text(fb, f, x, y, s);
    return x;
}
