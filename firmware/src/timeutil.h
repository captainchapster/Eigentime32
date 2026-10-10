/* Calendar maths and drift calculation. Portable C. */
#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t year;   /* 2000..2099 */
    uint8_t month;   /* 1..12 */
    uint8_t day;     /* 1..31 */
    uint8_t hour;    /* 0..23 */
    uint8_t min;     /* 0..59 */
    uint8_t sec;     /* 0..59 */
    uint8_t wday;    /* 0 = Sunday */
} dt_t;

int64_t dt_to_unix(const dt_t *d);
void dt_from_unix(int64_t t, dt_t *d);
uint8_t days_in_month(uint16_t year, uint8_t month);
bool dt_valid(const dt_t *d);

/* RV-3032 aging offset: 6-bit two's complement, 0.2384 ppm per step. */
#define RTC_OFFSET_STEP_PPB 238
#define RTC_OFFSET_MIN (-32)
#define RTC_OFFSET_MAX 31

/* Rate error in parts per billion: positive = the watch runs fast. */
int32_t drift_ppb(int64_t watch_minus_true_ms, int64_t elapsed_s);
/* New offset register value that cancels the measured drift.
 * Assumes a positive OFFSET speeds the clock up (verify on the first board). */
int8_t rtc_offset_correct(int8_t current, int32_t drift_ppb_measured);

#endif
