#include "face.h"

#include <stdio.h>

static const char *const WDAY[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const MON[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

#define TIME_Y (FACE_CY - 11)
#define DATE_Y 34
#define INFO_Y 84

static void two(char *b, unsigned v)
{
    b[0] = (char)('0' + v / 10 % 10);
    b[1] = (char)('0' + v % 10);
    b[2] = 0;
}

void face_render(gfx_fb_t *fb, const face_state_t *s)
{
    char hh[3], mm[3], buf[24];
    gfx_clear(fb);

    /* hour marks, plus a dot per elapsed second */
    for (int i = 0; i < 60; i++) {
        int a = i * 60;
        if (i % 5 == 0)
            gfx_radial(fb, FACE_CX, FACE_CY, i % 15 == 0 ? 55 : 57, 61, a, i % 15 == 0 ? 3 : 2);
        else if (s->time_valid && s->set_field == FIELD_NONE && i <= s->local.sec)
            gfx_radial(fb, FACE_CX, FACE_CY, 59, 60, a, 2);
    }

    /* time */
    if (s->time_valid || s->set_field != FIELD_NONE) {
        two(hh, s->local.hour);
        two(mm, s->local.min);
    } else {
        hh[0] = hh[1] = mm[0] = mm[1] = '-';
        hh[2] = mm[2] = 0;
    }
    int wh = gfx_text_width(&font_big, hh), wc = gfx_text_width(&font_big, ":");
    int wm = gfx_text_width(&font_big, mm);
    int x = FACE_CX - (wh + wc + wm + 4) / 2;
    int xh = x;
    x += gfx_text(fb, &font_big, x, TIME_Y, hh) + 2;
    if (!(s->sync_mode && s->local.sec & 1))
        gfx_text(fb, &font_big, x, TIME_Y, ":");
    x += wc + 2;
    int xm = x;
    gfx_text(fb, &font_big, x, TIME_Y, mm);

    /* date */
    int dx = 0, dw = 0, mx = 0, mw = 0, yx = 0, yw = 0;
    if (s->set_field >= FIELD_DAY) {
        /* while setting, show the full numeric date so each field can be highlighted */
        char d[3], m[3], y[6];
        two(d, s->local.day);
        two(m, s->local.month);
        snprintf(y, sizeof y, "%u", s->local.year);
        int w = gfx_text_width(&font_small, d) + gfx_text_width(&font_small, m) +
                gfx_text_width(&font_small, y) + 2 * (gfx_text_width(&font_small, ".") + 4);
        int p = FACE_CX - w / 2;
        dx = p; dw = gfx_text(fb, &font_small, p, DATE_Y, d); p += dw + 2;
        p += gfx_text(fb, &font_small, p, DATE_Y, ".") + 2;
        mx = p; mw = gfx_text(fb, &font_small, p, DATE_Y, m); p += mw + 2;
        p += gfx_text(fb, &font_small, p, DATE_Y, ".") + 2;
        yx = p; yw = gfx_text(fb, &font_small, p, DATE_Y, y);
    } else if (s->time_valid) {
        snprintf(buf, sizeof buf, "%s %u %s", WDAY[s->local.wday % 7], s->local.day,
                 MON[(s->local.month + 11) % 12]);
        gfx_text_centered(fb, &font_small, FACE_CX, DATE_Y, buf);
    }

    /* status line */
    const char *info = s->message;
    if (!info) {
        if (s->sync_mode)
            info = "PRESS AT :00";
        else if (s->set_field != FIELD_NONE)
            info = "SET";
        else if (!s->time_valid)
            info = "NOT SET";
        else {
            snprintf(buf, sizeof buf, "BAT %u%%", s->battery_pct);
            info = buf;
        }
    }
    gfx_text_centered(fb, &font_small, FACE_CX, INFO_Y, info);

    /* highlight the field being set */
    int fh = font_small.height;
    switch (s->set_field) {
    case FIELD_HOUR: gfx_invert_rect(fb, xh - 2, TIME_Y - 2, wh + 4, font_big.height + 4); break;
    case FIELD_MIN: gfx_invert_rect(fb, xm - 2, TIME_Y - 2, wm + 4, font_big.height + 4); break;
    case FIELD_DAY: gfx_invert_rect(fb, dx - 2, DATE_Y - 2, dw + 4, fh + 4); break;
    case FIELD_MONTH: gfx_invert_rect(fb, mx - 2, DATE_Y - 2, mw + 4, fh + 4); break;
    case FIELD_YEAR: gfx_invert_rect(fb, yx - 2, DATE_Y - 2, yw + 4, fh + 4); break;
    default: break;
    }
}
