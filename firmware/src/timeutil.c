#include "timeutil.h"

/* Days from civil, after Howard Hinnant's public-domain algorithms. */
static int64_t days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

int64_t dt_to_unix(const dt_t *d)
{
    return days_from_civil(d->year, d->month, d->day) * 86400 +
           d->hour * 3600 + d->min * 60 + d->sec;
}

void dt_from_unix(int64_t t, dt_t *d)
{
    int64_t z = t >= 0 ? t / 86400 : (t - 86399) / 86400;
    int32_t secs = (int32_t)(t - z * 86400);
    d->hour = (uint8_t)(secs / 3600);
    d->min = (uint8_t)(secs / 60 % 60);
    d->sec = (uint8_t)(secs % 60);
    d->wday = (uint8_t)((z % 7 + 11) % 7); /* 1970-01-01 was a Thursday */
    z += 719468;
    const int32_t era = (int32_t)((z >= 0 ? z : z - 146096) / 146097);
    const uint32_t doe = (uint32_t)(z - (int64_t)era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp = (5 * doy + 2) / 153;
    d->day = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
    d->month = (uint8_t)(mp < 10 ? mp + 3 : mp - 9);
    d->year = (uint16_t)((int32_t)yoe + era * 400 + (d->month <= 2));
}

uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
        return 29;
    return dim[(month - 1) % 12];
}

bool dt_valid(const dt_t *d)
{
    return d->year >= 2000 && d->year <= 2099 && d->month >= 1 && d->month <= 12 &&
           d->day >= 1 && d->day <= days_in_month(d->year, d->month) && d->hour < 24 &&
           d->min < 60 && d->sec < 60;
}

int32_t drift_ppb(int64_t watch_minus_true_ms, int64_t elapsed_s)
{
    if (elapsed_s <= 0)
        return 0;
    int64_t v = watch_minus_true_ms * 1000000 / elapsed_s;
    if (v > INT32_MAX)
        v = INT32_MAX;
    if (v < INT32_MIN)
        v = INT32_MIN;
    return (int32_t)v;
}

int8_t rtc_offset_correct(int8_t current, int32_t measured)
{
    /* round to the nearest step */
    int32_t steps = (measured >= 0 ? measured + RTC_OFFSET_STEP_PPB / 2
                                   : measured - RTC_OFFSET_STEP_PPB / 2) / RTC_OFFSET_STEP_PPB;
    int32_t v = current - steps;
    if (v < RTC_OFFSET_MIN)
        v = RTC_OFFSET_MIN;
    if (v > RTC_OFFSET_MAX)
        v = RTC_OFFSET_MAX;
    return (int8_t)v;
}
