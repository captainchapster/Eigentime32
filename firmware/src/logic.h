/* Hardware-free pieces of the app, so they can be tested on the host. */
#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#include "timeutil.h"

uint8_t battery_pct(uint16_t mv);

/* Steps one field of a local date/time up by one, wrapping within its range. */
void field_increment(dt_t *d, int field);

typedef struct {
    int32_t drift_ppb;     /* measured rate error, positive = watch fast */
    int8_t new_offset;     /* RV-3032 offset to program */
    bool apply_offset;     /* the measurement spans long enough to trust */
} sync_result_t;

/* A reference sync: the watch read watch_ms when the reference said true_ms.
 * last_sync_s is the previous reference sync (0 = none). */
sync_result_t sync_evaluate(int64_t watch_ms, int64_t true_ms, int64_t last_sync_s,
                            int8_t current_offset, int64_t min_span_s);

/* Button sync at :00 on a reference clock: the nearest whole minute to watch_ms. */
int64_t round_to_minute_ms(int64_t watch_ms);

/* Writes "+1.25" style text for a value in thousandths into buf (at least 12 bytes). */
void fmt_milli(char *buf, int32_t v);

#endif
