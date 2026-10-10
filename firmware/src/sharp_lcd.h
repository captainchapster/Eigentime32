/* Sharp LS013B7DH03 memory LCD (EXTMODE high: VCOM toggled by EXTCOMIN from the RTC). */
#ifndef SHARP_LCD_H
#define SHARP_LCD_H

#include <stdbool.h>

#include "gfx.h"

void sharp_on(void);                        /* power-up sequence, ends with a cleared, enabled panel */
void sharp_off(void);                       /* power-down sequence */
/* Sends the lines that differ from the last frame sent (all of them after sharp_on). */
void sharp_flush(const gfx_fb_t *fb);

#endif
