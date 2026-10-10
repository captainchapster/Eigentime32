/* Host tests for the hardware-free code. make -C firmware host */
#include <stdio.h>
#include <string.h>

#include "logic.h"
#include "ndef.h"
#include "timeutil.h"

static int fails;
#define CHECK(c)                                                                 \
    do {                                                                         \
        if (!(c)) {                                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                  \
            fails++;                                                             \
        }                                                                        \
    } while (0)

static void test_calendar(void)
{
    dt_t d;
    dt_from_unix(0, &d);
    CHECK(d.year == 1970 && d.month == 1 && d.day == 1 && d.wday == 4);
    dt_from_unix(1791628875, &d); /* 2026-10-10 10:41:15 UTC, a Saturday */
    CHECK(d.year == 2026 && d.month == 10 && d.day == 10 && d.hour == 10 && d.min == 41 &&
          d.sec == 15 && d.wday == 6);
    dt_from_unix(951782400, &d);  /* 2000-02-29 */
    CHECK(d.year == 2000 && d.month == 2 && d.day == 29);
    /* round trip every 3 days 11 h 7 min 13 s from 2000 to 2099 */
    for (int64_t t = 946684800; t < 4102444800LL; t += 299233) {
        dt_from_unix(t, &d);
        if (dt_to_unix(&d) != t || !dt_valid(&d)) {
            CHECK(0);
            break;
        }
    }
    CHECK(days_in_month(2100, 2) == 28 && days_in_month(2024, 2) == 29);
}

static void test_drift(void)
{
    /* 10 s fast after a year is +317 ppb */
    CHECK(drift_ppb(10000, 31536000) == 317);
    CHECK(rtc_offset_correct(0, 317) == -1);   /* 1.33 steps -> 1 */
    CHECK(rtc_offset_correct(0, -400) == 2);   /* -1.68 steps -> 2 */
    CHECK(rtc_offset_correct(-31, 5000) == -32);
    CHECK(rtc_offset_correct(30, -5000) == 31);

    sync_result_t r = sync_evaluate(1000000000000LL + 5000, 1000000000000LL, 0, 0, 7 * 86400);
    CHECK(!r.apply_offset);                    /* no previous sync */
    r = sync_evaluate(1000000000000LL + 5000, 1000000000000LL, 1000000000 - 2 * 86400, 0, 7 * 86400);
    CHECK(!r.apply_offset);                    /* span too short */
    r = sync_evaluate(1000000000000LL + 15000, 1000000000000LL, 1000000000 - 365 * 86400, 3, 7 * 86400);
    CHECK(r.drift_ppb == 475 && r.new_offset == 1 && r.apply_offset);

    CHECK(round_to_minute_ms(60000 * 10 + 29999) == 600000);
    CHECK(round_to_minute_ms(60000 * 10 + 30000) == 660000);
    CHECK(round_to_minute_ms(60000 * 10 - 2000) == 600000);
}

static void test_format(void)
{
    char b[16];
    fmt_milli(b, 1250);
    CHECK(strcmp(b, "+1.25") == 0);
    fmt_milli(b, -3004);
    CHECK(strcmp(b, "-3.00") == 0);
    fmt_milli(b, 999);
    CHECK(strcmp(b, "+1.00") == 0);
    fmt_milli(b, 0);
    CHECK(strcmp(b, "+0.00") == 0);
    CHECK(battery_pct(3100) == 100 && battery_pct(2850) == 65 && battery_pct(2100) == 0);
}

static void test_fields(void)
{
    dt_t d = {2027, 1, 31, 23, 59, 0, 0};
    field_increment(&d, 3);               /* month: Jan 31 -> Feb 28 */
    CHECK(d.month == 2 && d.day == 28);
    field_increment(&d, 0);
    CHECK(d.hour == 0);
    field_increment(&d, 1);
    CHECK(d.min == 0);
    d.year = 2099;
    field_increment(&d, 4);
    CHECK(d.year == 2024);
}

static void test_ndef(void)
{
    uint8_t buf[200];
    size_t n = ndef_build_uri(buf, sizeof buf, 512, "example.org/sync/?t=1");
    CHECK(n == 4 + 2 + 4 + 1 + 21 + 1);
    CHECK(buf[0] == 0xE1 && buf[2] == 64 && buf[4] == 0x03 && buf[6] == 0xD1 && buf[n - 1] == 0xFE);
    int64_t ms;
    int16_t tz;
    CHECK(!ndef_find_sync(buf, n, &ms, &tz));

    /* what Web NFC writes for {recordType: "text", data: "ET32 T=... Z=..."} */
    const char *txt = "ET32 T=1791628875123 Z=60";
    uint8_t m[128] = {0xE1, 0x40, 0x40, 0x00, 0x03, 0};
    size_t tl = strlen(txt), i = 6;
    m[i++] = 0xD1;
    m[i++] = 1;
    m[i++] = (uint8_t)(1 + 2 + tl);
    m[i++] = 'T';
    m[i++] = 2;
    m[i++] = 'e';
    m[i++] = 'n';
    memcpy(&m[i], txt, tl);
    i += tl;
    m[5] = (uint8_t)(i - 6);
    m[i++] = 0xFE;
    CHECK(ndef_find_sync(m, sizeof m, &ms, &tz) && ms == 1791628875123LL && tz == 60);

    /* a URI record first, then the text record */
    uint8_t m2[160] = {0xE1, 0x40, 0x40, 0x00, 0x03, 0};
    i = 6;
    m2[i++] = 0x91; /* MB SR, not ME */
    m2[i++] = 1;
    m2[i++] = 4;
    m2[i++] = 'U';
    m2[i++] = 4;
    m2[i++] = 'a';
    m2[i++] = '.';
    m2[i++] = 'b';
    m2[i++] = 0x51; /* ME SR */
    m2[i++] = 1;
    m2[i++] = (uint8_t)(1 + 2 + tl);
    m2[i++] = 'T';
    m2[i++] = 2;
    m2[i++] = 'e';
    m2[i++] = 'n';
    memcpy(&m2[i], "ET32 T=5 Z=-300", 15);
    m2[i - 5] = (uint8_t)(1 + 2 + 15);
    i += 15;
    m2[5] = (uint8_t)(i - 6);
    m2[i++] = 0xFE;
    CHECK(ndef_find_sync(m2, sizeof m2, &ms, &tz) && ms == 5 && tz == -300);

    /* malformed: truncated length must not read past the buffer */
    m[5] = 200;
    CHECK(!ndef_find_sync(m, 64, &ms, &tz));
    /* out-of-range zone offset is rejected */
    memcpy(&m2[i - 1 - 15], "ET32 T=5 Z=9999", 15);
    CHECK(!ndef_find_sync(m2, sizeof m2, &ms, &tz));
}

int main(void)
{
    test_calendar();
    test_drift();
    test_format();
    test_fields();
    test_ndef();
    printf("%s (%d failures)\n", fails ? "FAILED" : "all host tests passed", fails);
    return fails != 0;
}
