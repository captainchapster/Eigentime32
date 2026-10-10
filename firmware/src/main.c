/* Eigentime32: wake on the lid, show the time, sleep. */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "config.h"
#include "face.h"
#include "logic.h"
#include "ndef.h"
#include "rv3032.h"
#include "settings.h"
#include "sharp_lcd.h"
#include "st25dv.h"

static settings_t cfg;
static gfx_fb_t fb;
static face_state_t face;

static bool display_on, timed_out;
static uint32_t idle_s, msg_s;      /* seconds counted from RTC ticks while the display is on */
static char msg[24];
static uint32_t last_status_write;  /* RTC seconds (low word) of the last tag status write */
static bool status_dirty = true;

/* ---------------------------------------------------------------- time */
static bool now_utc_ms(int64_t *ms)
{
    dt_t t;
    uint8_t h;
    if (!rv3032_read(&t, &h))
        return false;
    *ms = dt_to_unix(&t) * 1000 + h * 10;
    return true;
}

static bool set_utc_ms_at_boundary(int64_t ms_now_ref)
{
    /* write the next whole second exactly when the reference clock reaches it */
    int64_t next = (ms_now_ref / 1000 + 1) * 1000;
    delay_ms((uint32_t)(next - ms_now_ref));
    dt_t t;
    dt_from_unix(next / 1000, &t);
    return rv3032_write(&t);
}

static void show_message(const char *m)
{
    strncpy(msg, m, sizeof msg - 1);
    msg[sizeof msg - 1] = 0;
    msg_s = MESSAGE_S;
}

/* ---------------------------------------------------------------- NFC status link */
static void write_status(void)
{
    int64_t ms = 0;
    int8_t off = 0;
    bool have_time = cfg.time_set && now_utc_ms(&ms);
    rv3032_get_offset(&off);
    char url[160];
    snprintf(url, sizeof url, "%s?t=%lu&z=%ld&o=%d&d=%ld&b=%u&s=%lu&v=1", STATUS_URL,
             have_time ? (unsigned long)(ms / 1000) : 0ul, (long)cfg.tz_min, off,
             (long)cfg.drift_ppb, battery_mv(), (unsigned long)settings_last_sync(&cfg));
    uint8_t buf[200];
    size_t n = ndef_build_uri(buf, sizeof buf, ST25DV_USER_BYTES, url);
    if (n && st25dv_open()) {
        st25dv_write(0, buf, n);
        st25dv_close();
        status_dirty = false;
        last_status_write = (uint32_t)(ms / 1000);
    }
}

/* A phone wrote to the tag. If it left a sync record, take the time from it. */
static void handle_nfc(uint32_t event_ms)
{
    delay_ms(300); /* let the phone finish writing */
    uint8_t mem[128];
    if (!st25dv_open())
        return;
    bool ok = st25dv_read(0, mem, sizeof mem);
    st25dv_close();
    int64_t t_ms;
    int16_t tz;
    if (!ok || !ndef_find_sync(mem, sizeof mem, &t_ms, &tz))
        return;

    int64_t watch_ms;
    bool had_time = cfg.time_set && now_utc_ms(&watch_ms);
    if (had_time) {
        watch_ms -= millis() - event_ms; /* the watch's reading at the moment of the write */
        int8_t off = 0;
        rv3032_get_offset(&off);
        sync_result_t r = sync_evaluate(watch_ms, t_ms, settings_last_sync(&cfg), off, MIN_DRIFT_SPAN_S);
        if (r.apply_offset) {
            cfg.drift_ppb = r.drift_ppb;
            rv3032_set_offset(r.new_offset);
        }
        char d[12];
        fmt_milli(d, (int32_t)(watch_ms - t_ms));
        snprintf(msg, sizeof msg, "SYNC %sS", d);
        show_message(msg);
    } else {
        show_message("TIME SET");
    }
    set_utc_ms_at_boundary(t_ms + (millis() - event_ms));
    cfg.tz_min = tz;
    cfg.time_set = 1;
    settings_set_last_sync(&cfg, t_ms / 1000);
    settings_save(&cfg);
    status_dirty = true;
    write_status(); /* also removes the sync record */
    board_drop_events(EV_NFC); /* pulses from the rest of that phone write */
}

/* ---------------------------------------------------------------- display */
static void refresh(void)
{
    int64_t ms;
    face.time_valid = cfg.time_set && now_utc_ms(&ms);
    if (face.time_valid && face.set_field == FIELD_NONE)
        dt_from_unix(ms / 1000 + cfg.tz_min * 60, &face.local);
    face.message = msg_s ? msg : NULL;
    face_render(&fb, &face);
    sharp_flush(&fb);
}

static void display_start(void)
{
    face.battery_pct = battery_pct(battery_mv());
    face.set_field = FIELD_NONE;
    face.sync_mode = false;
    sharp_on();
    refresh();
    rv3032_clkout(true);                 /* EXTCOMIN after DISP is high */
    rv3032_tick(true, true);
    display_on = true;
    idle_s = 0;
}

static void display_stop(void)
{
    rv3032_tick(false, true);
    rv3032_clkout(false);
    sharp_off();
    display_on = false;
    face.set_field = FIELD_NONE;
    face.sync_mode = false;
    msg_s = 0;
    int64_t ms;
    if (cfg.time_set && now_utc_ms(&ms) &&
        (status_dirty || (uint32_t)(ms / 1000) - last_status_write >= STATUS_REFRESH_S))
        write_status();
}

/* ---------------------------------------------------------------- buttons */
/* Waits for release; returns true for a long press. Calls repeat() while held past the long-press time. */
static bool press(bool (*down)(void), void (*repeat)(void))
{
    delay_ms(20);
    uint32_t t0 = millis(), last = t0;
    bool lng = false;
    while (down()) {
        uint32_t now = millis();
        if (!lng && now - t0 >= LONG_PRESS_MS) {
            lng = true;
            last = now;
            if (!repeat)
                break;
        }
        if (lng && repeat && now - last >= REPEAT_MS) {
            last = now;
            repeat();
        }
        delay_ms(10);
    }
    return lng;
}

static void bump(void)
{
    field_increment(&face.local, face.set_field);
    face_render(&fb, &face);
    sharp_flush(&fb);
}

static void commit_set(void)
{
    face.local.sec = 0;
    int64_t utc = dt_to_unix(&face.local) - cfg.tz_min * 60;
    dt_t t;
    dt_from_unix(utc, &t);
    if (rv3032_write(&t)) {
        cfg.time_set = 1;
        settings_set_last_sync(&cfg, 0); /* a hand-set time is no reference for drift */
        settings_save(&cfg);
        status_dirty = true;
        show_message("TIME SET");
    }
}

static void button1(void)
{
    if (face.set_field != FIELD_NONE) {
        if (press(btn1_down, NULL)) {
            face.set_field = FIELD_NONE; /* long press cancels */
            show_message("CANCELLED");
        } else if (++face.set_field == FIELD_COUNT) {
            face.set_field = FIELD_NONE;
            commit_set();
        }
    } else if (face.sync_mode) {
        press(btn1_down, NULL);
        face.sync_mode = false;
        show_message("CANCELLED");
    } else if (press(btn1_down, NULL)) {
        face.set_field = FIELD_HOUR;     /* long press: set the time by hand */
        if (!face.time_valid) {
            dt_from_unix(1767225600 + cfg.tz_min * 60, &face.local); /* 2026-01-01 */
        }
    } else {
        char d[12];
        fmt_milli(d, cfg.drift_ppb);     /* ppb -> ppm with 3 decimals */
        snprintf(msg, sizeof msg, "%s PPM", d);
        show_message(msg);
    }
}

static void button2(void)
{
    if (face.set_field != FIELD_NONE) {
        if (!press(btn2_down, bump))
            bump();
    } else if (face.sync_mode) {
        /* the reference clock reads :00 right now */
        int64_t watch_ms;
        if (now_utc_ms(&watch_ms)) {
            int64_t local = watch_ms + cfg.tz_min * 60000LL;
            int64_t true_ms = round_to_minute_ms(local) - cfg.tz_min * 60000LL;
            int8_t off = 0;
            rv3032_get_offset(&off);
            sync_result_t r = sync_evaluate(watch_ms, true_ms, settings_last_sync(&cfg), off,
                                            MIN_DRIFT_SPAN_S);
            if (r.apply_offset) {
                cfg.drift_ppb = r.drift_ppb;
                rv3032_set_offset(r.new_offset);
            }
            dt_t t;
            dt_from_unix(true_ms / 1000, &t);
            rv3032_write(&t);
            settings_set_last_sync(&cfg, true_ms / 1000);
            settings_save(&cfg);
            status_dirty = true;
            char d[12];
            fmt_milli(d, (int32_t)(watch_ms - true_ms));
            snprintf(msg, sizeof msg, "ADJ %sS", d);
            show_message(msg);
        }
        face.sync_mode = false;
        press(btn2_down, NULL);
    } else if (press(btn2_down, NULL)) {
        if (cfg.time_set)
            face.sync_mode = true;       /* long press: sync at :00 against a reference */
    } else {
        snprintf(msg, sizeof msg, "%u MV", battery_mv());
        show_message(msg);
    }
}

/* ---------------------------------------------------------------- main */
int main(void)
{
    board_init();
    settings_load(&cfg);
    face.set_field = FIELD_NONE;

    rv3032_status_t st = {0};
    if (rv3032_init(&st) && (st.power_on_reset || st.voltage_low)) {
        cfg.time_set = 0;                /* cell swap: the RTC lost the time */
        settings_set_last_sync(&cfg, 0);
        settings_save(&cfg);
    }
    if (st25dv_open()) {
        st25dv_config_gpo();
        st25dv_close();
    }
    write_status();

    for (;;) {
        uint32_t ev = board_take_events();
        uint32_t ev_ms = millis();

        if (ev & EV_NFC)
            handle_nfc(ev_ms);

        bool open = lid_open();
        if (!open) {
            timed_out = false;
            if (display_on)
                display_stop();
        } else if (!display_on && (!timed_out || (ev & (EV_BTN1 | EV_BTN2)))) {
            display_start();
            ev &= ~(EV_BTN1 | EV_BTN2);  /* the press that woke it does nothing else */
        }

        if (display_on) {
            if (ev & EV_RTC) {
                rv3032_ack_tick();
                idle_s++;
                if (msg_s)
                    msg_s--;
            }
            if (ev & EV_BTN1) {
                idle_s = 0;
                button1();
            }
            if (ev & EV_BTN2) {
                idle_s = 0;
                button2();
            }
            if (idle_s >= DISPLAY_TIMEOUT_S && face.set_field == FIELD_NONE) {
                display_stop();
                timed_out = true;
            } else if (ev) {
                refresh();
            }
        } else if (ev & EV_RTC) {
            rv3032_ack_tick();
        }

        board_sleep();
    }
}
