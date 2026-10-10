#include "settings.h"

#include <string.h>

#include "board.h"

#define MAGIC 0x45543332u /* "ET32" */

void settings_load(settings_t *s)
{
    eeprom_read(0, s, sizeof *s);
    if (s->magic != MAGIC) {
        memset(s, 0, sizeof *s);
        s->magic = MAGIC;
    }
}

void settings_save(const settings_t *s)
{
    eeprom_write_words(0, (const uint32_t *)s, sizeof *s / 4);
}

int64_t settings_last_sync(const settings_t *s) { return (int64_t)((uint64_t)s->sync_hi << 32 | s->sync_lo); }

void settings_set_last_sync(settings_t *s, int64_t utc)
{
    s->sync_lo = (uint32_t)utc;
    s->sync_hi = (uint32_t)((uint64_t)utc >> 32);
}
