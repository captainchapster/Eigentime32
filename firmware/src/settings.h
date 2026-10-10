/* Persistent settings in the STM32's data EEPROM. */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t magic;
    int32_t tz_min;            /* local offset from UTC, minutes (from the last phone sync) */
    uint32_t sync_lo, sync_hi; /* UTC seconds of the last reference sync (0 = none) */
    int32_t drift_ppb;         /* rate error measured at the last sync, before correction */
    uint32_t time_set;         /* the RTC holds a real time */
} settings_t;

void settings_load(settings_t *s);
void settings_save(const settings_t *s);
int64_t settings_last_sync(const settings_t *s);
void settings_set_last_sync(settings_t *s, int64_t utc);

#endif
