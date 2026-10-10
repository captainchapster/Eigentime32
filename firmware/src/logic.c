#include "logic.h"

#include "face.h"

uint8_t battery_pct(uint16_t mv)
{
    /* CR2032 under a light load: flat near 3.0 V for most of its life, then a knee */
    static const struct { uint16_t mv; uint8_t pct; } pts[] = {
        {3000, 100}, {2900, 80}, {2800, 50}, {2700, 30}, {2600, 15}, {2400, 5}, {2200, 0},
    };
    if (mv >= pts[0].mv)
        return 100;
    for (unsigned i = 1; i < sizeof pts / sizeof pts[0]; i++)
        if (mv >= pts[i].mv)
            return (uint8_t)(pts[i].pct + (mv - pts[i].mv) * (pts[i - 1].pct - pts[i].pct) /
                                             (pts[i - 1].mv - pts[i].mv));
    return 0;
}

void field_increment(dt_t *d, int field)
{
    switch (field) {
    case FIELD_HOUR: d->hour = (uint8_t)((d->hour + 1) % 24); break;
    case FIELD_MIN: d->min = (uint8_t)((d->min + 1) % 60); break;
    case FIELD_DAY: d->day = (uint8_t)(d->day % days_in_month(d->year, d->month) + 1); break;
    case FIELD_MONTH: d->month = (uint8_t)(d->month % 12 + 1); break;
    case FIELD_YEAR: d->year = (uint16_t)(d->year >= 2099 ? 2024 : d->year + 1); break;
    default: break;
    }
    if (d->day > days_in_month(d->year, d->month))
        d->day = days_in_month(d->year, d->month);
    /* keep the weekday consistent */
    dt_t tmp = *d;
    dt_from_unix(dt_to_unix(&tmp), &tmp);
    d->wday = tmp.wday;
}

sync_result_t sync_evaluate(int64_t watch_ms, int64_t true_ms, int64_t last_sync_s,
                            int8_t current_offset, int64_t min_span_s)
{
    sync_result_t r = {0, current_offset, false};
    int64_t span = last_sync_s > 0 ? true_ms / 1000 - last_sync_s : 0;
    if (span >= min_span_s) {
        r.drift_ppb = drift_ppb(watch_ms - true_ms, span);
        r.new_offset = rtc_offset_correct(current_offset, r.drift_ppb);
        r.apply_offset = r.new_offset != current_offset;
    }
    return r;
}

int64_t round_to_minute_ms(int64_t watch_ms)
{
    int64_t m = (watch_ms + 30000) / 60000;
    return m * 60000;
}

void fmt_milli(char *buf, int32_t v)
{
    char *p = buf;
    *p++ = v < 0 ? '-' : '+';
    uint32_t a = (uint32_t)(v < 0 ? -(int64_t)v : v);
    uint32_t ip = a / 1000, fp = (a % 1000 + 5) / 10;
    if (fp == 100) {
        ip++;
        fp = 0;
    }
    char tmp[11];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + ip % 10);
        ip /= 10;
    } while (ip);
    while (n)
        *p++ = tmp[--n];
    *p++ = '.';
    *p++ = (char)('0' + fp / 10);
    *p++ = (char)('0' + fp % 10);
    *p = 0;
}
