/* Watch face rendering. Portable C. */
#ifndef FACE_H
#define FACE_H

#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"
#include "timeutil.h"

/* The round window shows a circle of radius ~63 px centred on the panel. */
#define FACE_CX 64
#define FACE_CY 64

enum face_field { FIELD_NONE = -1, FIELD_HOUR, FIELD_MIN, FIELD_DAY, FIELD_MONTH, FIELD_YEAR, FIELD_COUNT };

typedef struct {
    dt_t local;           /* local time to show */
    bool time_valid;      /* false after a cell swap until the time is set */
    uint8_t battery_pct;
    int8_t set_field;     /* enum face_field: field being edited, FIELD_NONE when not setting */
    bool sync_mode;       /* waiting for the user to press at :00 on a reference clock */
    const char *message;  /* short status line, NULL for the battery */
} face_state_t;

void face_render(gfx_fb_t *fb, const face_state_t *s);

#endif
