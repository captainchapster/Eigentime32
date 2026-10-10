/* Host build: renders watch faces to PBM files for a quick look.
 * make -C firmware preview  -> firmware/build/host/face_*.pbm (then *.png via tools/pbm2png.py) */
#include <stdio.h>
#include <string.h>

#include "face.h"

static void save(const gfx_fb_t *fb, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return;
    }
    fprintf(f, "P1\n%d %d\n", GFX_W, GFX_H);
    for (int y = 0; y < GFX_H; y++) {
        for (int x = 0; x < GFX_W; x++)
            fputc(fb->px[y][x >> 3] & (1u << (x & 7)) ? '1' : '0', f);
        fputc('\n', f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[256];
    gfx_fb_t fb;
    face_state_t s;
    memset(&s, 0, sizeof s);
    s.set_field = FIELD_NONE;
    dt_from_unix(1791628875, &s.local); /* 2026-10-10 10:41:15 */
    s.time_valid = true;
    s.battery_pct = 87;

    face_render(&fb, &s);
    snprintf(path, sizeof path, "%s/face_normal.pbm", dir);
    save(&fb, path);

    s.set_field = FIELD_MIN;
    face_render(&fb, &s);
    snprintf(path, sizeof path, "%s/face_set_minute.pbm", dir);
    save(&fb, path);

    s.set_field = FIELD_MONTH;
    face_render(&fb, &s);
    snprintf(path, sizeof path, "%s/face_set_month.pbm", dir);
    save(&fb, path);

    s.set_field = FIELD_NONE;
    s.sync_mode = true;
    face_render(&fb, &s);
    snprintf(path, sizeof path, "%s/face_sync.pbm", dir);
    save(&fb, path);

    s.sync_mode = false;
    s.time_valid = false;
    face_render(&fb, &s);
    snprintf(path, sizeof path, "%s/face_unset.pbm", dir);
    save(&fb, path);
    return 0;
}
